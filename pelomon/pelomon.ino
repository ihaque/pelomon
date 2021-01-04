/* Main sketch for the PeloMon.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <SPI.h>

#include "settings.h"

// Requires "Adafruit BluefruitLE nRF51" library
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

uint8_t LOG_LEVEL;

#include "logger.h"
#include "BLECyclingGatt.h"
#include "resistance_lut.h"
#include "peloton.h"
#include "RideStatus.h"

#ifndef MIN
#define MIN(x,y) (x) < (y) ? (x) : (y)
#endif

void serial_log_messagepair_text(void);

/* DATA TYPES
 *
 */

/* COMMUNICATIONS
 *
 */

/* GLOBALS - MCU state
 *
 * note - log level global is at top of sketch
 */

#define HU_MSG_BUF_LEN 4
#define BIKE_MSG_BUF_LEN 15

uint8_t hu_buf[HU_MSG_BUF_LEN];
uint8_t bike_buf[BIKE_MSG_BUF_LEN];
uint8_t hu_buf_bytes;
uint8_t bike_buf_bytes;
uint8_t running_checksum;
unsigned long last_status_sent;
unsigned long last_time_messages_seen;
bool boot_sequence_complete;

// Set up an ISR on an arbitrary point in timer0 which ticks
// over at about 1KHz. Use this to time-limit our wait for
// bike responses and ensure user responsiveness.
// This variable is modified in an ISR so needs to be volatile.
volatile uint8_t bike_wait_ms_remaining;
SIGNAL(TIMER0_COMPA_vect) {
    bike_wait_ms_remaining--;
}
inline void enable_bike_timeout(void) {
    // Arbitrary value. Just need interrupt to fire once per timer cycle.
    OCR0A = 0xB0;
    TIMSK0 |= _BV(OCIE0A);
}
inline void disable_bike_timeout(void) {
    TIMSK0 &= ~(_BV(OCIE0A));
}

Logger logger;
PelotonProxy peloton;
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
BLECyclingPower power_service(ble, logger);
RideStatus ride_status(logger);
ResistanceLUT resistance_lut(logger);

#define ENABLE_RINGBUF
#include "ringbuf.h"

void reboot(void) {
    // Reset board using the watchdog timer
    wdt_disable();
    wdt_enable(WDTO_15MS);
    while (1);
}

void setup() {
    // Initialize I/Os and communications first
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_LOW_FORCE_SIM, INPUT_PULLUP);

    // DEBUG
    pinMode(PIN_STATE_READ_HU, OUTPUT);
    pinMode(PIN_STATE_READ_BIKE, OUTPUT);
    pinMode(PIN_STATE_PROC_MSG, OUTPUT);
    pinMode(PIN_STATE_HANDLE_CMD, OUTPUT);
    pinMode(PIN_STATE_LISTEN_HU, OUTPUT);
    pinMode(PIN_STATE_LISTEN_BIKE, OUTPUT);
    digitalWrite(PIN_STATE_READ_HU, LOW);
    digitalWrite(PIN_STATE_READ_BIKE, LOW);
    digitalWrite(PIN_STATE_PROC_MSG, LOW);
    digitalWrite(PIN_STATE_HANDLE_CMD, LOW);
    digitalWrite(PIN_STATE_LISTEN_HU, LOW);
    digitalWrite(PIN_STATE_LISTEN_BIKE, LOW);

    // Set LED high while initializing
    digitalWrite(LED_BUILTIN, HIGH);

    // Set up initial log level
    LOG_LEVEL = LOG_LEVEL_INFO;

    // Initialize serial ports
    Serial.begin(230400); // Communicate with PC, if exists, at 230.4kbps
    // Give Serial three seconds to connect then charge ahead
    delay(3000);

    logger.println(F("Initializing BLE module..."));
    // Initialize BLE module
    const bool ble_verbose = LOG_LEVEL > LOG_LEVEL_INFO;
    if ( !ble.begin(ble_verbose) ) {
        logger.println(F("BLE init failed"));
        while (1);
    }
    ble.echo(false);

    logger.set_ble(&ble);

    logger.println(F("Communications initialized"));

    // Initialize buffers and state
    memset(hu_buf, 0, HU_MSG_BUF_LEN);
    memset(bike_buf, 0, BIKE_MSG_BUF_LEN);
    hu_buf_bytes = bike_buf_bytes = running_checksum = 0;
    last_time_messages_seen = 0;
    boot_sequence_complete = false;
    init_ringbuf();

    resistance_lut.initialize();
    ride_status.initialize();

    // Decide whether to use real bike or simulator
    // Simulate if requested in software or forced in hardware.
    bool use_simulator = (EEPROM.read(EEPROM_FORCE_SIMULATION_AT_STARTUP)
                          || (digitalRead(PIN_LOW_FORCE_SIM) == LOW)
                          || 0);
    // Clear the force flag for next bootup
    if (use_simulator) EEPROM.update(EEPROM_FORCE_SIMULATION_AT_STARTUP,
                                     false);

    if (LOG_LEVEL >= LOG_LEVEL_INFO) {
        if (use_simulator) logger.println(F("Simulator requested, using sim"));
    }
    peloton.initialize(use_simulator);

    power_service.initialize();

    // Initialize state machine
    last_status_sent = millis() - BT_UPDATE_INTERVAL_MILLIS;

    // Set LED low once bootup is complete
    digitalWrite(LED_BUILTIN, LOW);
}

bool receive_message_pair(void) {
    bool hu_message_complete = false, bike_message_complete = false;
    digitalWrite(PIN_STATE_READ_HU, HIGH);
    peloton.hu_listen();
    if (peloton.hu_available() == 0) {
        digitalWrite(PIN_STATE_READ_HU, LOW);
        return hu_message_complete && bike_message_complete;
    }

    // Read HU message
    unsigned long receive_start;
    while (peloton.hu_available()) {
        receive_start = millis();
        uint8_t next_byte = peloton.hu_read();
        if (next_byte == 0xFE || next_byte == 0xF5 || next_byte == 0xF7 ||
            hu_buf_bytes > (HU_MSG_BUF_LEN - 1)) {
            // Reset - starting a new message, or overflow
            // if this byte isn't the checksum
            if (next_byte != running_checksum)
                hu_buf_bytes = 0;
        }
        hu_buf[hu_buf_bytes++] = (uint8_t) next_byte;
        running_checksum += next_byte;

        if (next_byte == 0xF6) {
            // End message
            peloton.bike_listen();
            digitalWrite(PIN_STATE_READ_HU, LOW);
            digitalWrite(PIN_STATE_READ_BIKE, HIGH);
            running_checksum = 0;
            hu_message_complete = true;
            break;
        }
    }

    // Read bike message with no interruptions since HU completion
    // Enable timeout timer
    enable_bike_timeout();
    while (!bike_message_complete) {
        // Wait a max of 10-11ms at each byte
        // (might be 10 if the timer ticks over immediately after we assign)
        bike_wait_ms_remaining = 11;
        while (!peloton.bike_available()) {
            // If we have waited too long for the bike to respond, bail.
            if (bike_wait_ms_remaining == 0) {
                disable_bike_timeout();
                return false;
            }
        }
        uint8_t next_byte = peloton.bike_read();

        if (next_byte == 0xF1 ||
            bike_buf_bytes > (BIKE_MSG_BUF_LEN - 1)) {
            // Reset - starting a new message; or overflow
            // as long as this byte isn't the checksum
            if (next_byte != running_checksum)
                bike_buf_bytes = 0;
        }
        bike_buf[bike_buf_bytes++] = (uint8_t) next_byte;
        running_checksum += next_byte;

        if (next_byte == 0xF6) {
            // End message
            peloton.hu_listen();
            digitalWrite(PIN_STATE_READ_BIKE, LOW);
            running_checksum = 0;
            bike_message_complete = true;
        }
    }
    disable_bike_timeout();

    return hu_message_complete && bike_message_complete;
}

// Returns true if the message seen indicates that the bootup sequence is done.
bool process_message_pair(void) {
    digitalWrite(PIN_STATE_PROC_MSG, HIGH);
    char logbuf[32];
    const unsigned long process_start = micros();
    bool updated_ride_status = false;
    bool done_with_boot = false;

    // Parse messages in the buffers
    HUMessage hu_msg(hu_buf, hu_buf_bytes);
    BikeMessage bike_msg(bike_buf, bike_buf_bytes);

    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        snprintf_P(logbuf, 32,
                   PSTR("hu valid:%hhu,bike valid:%hhu\n"),
                   (uint8_t) hu_msg.is_valid,
                   (uint8_t) bike_msg.is_valid);
        logger.print(logbuf);
        serial_log_messagepair_text();
    }

    if (hu_msg.is_valid && bike_msg.is_valid) {
        if (hu_msg.packet_type == READ_RESISTANCE_TABLE) {
            add_ringbuf();
            resistance_lut.update_entry(bike_msg.value, hu_msg.request);
            // Sync to EEPROM once we get all the resistance values
            if (hu_msg.request == 0x1E) {
                done_with_boot = true;
                resistance_lut.sync_to_eeprom();
                logger.print(F("Rcvd all RLUT msgs,syncing\n"));
                resistance_lut.serial_status_text();
            }

        } else if (bike_msg.request == BIKE_ID ||
                   hu_msg.packet_type == STARTUP_UNKNOWN) {
            // Do nothing on the two startup packets
            add_ringbuf();

        } else {
            // Update internal ride status state
            ride_status.update(bike_msg, resistance_lut);
            updated_ride_status = true;
            done_with_boot = true;
        }
    } else {
        add_ringbuf();
    }

    // Update BLE gadget state
    // Takes 24us + time to update GATTs = 15ms
    const unsigned long bt_start = micros();
    const unsigned long current_time = millis();
    if (current_time - last_status_sent >= BT_UPDATE_INTERVAL_MILLIS) {
        last_status_sent = current_time;
        power_service.update(ride_status.integral_crank_revolutions(),
                             ride_status.last_crank_rev_ts_millis(),
                             ride_status.integral_wheel_revolutions(),
                             ride_status.last_wheel_rev_ts_millis(),
                             ride_status.current_watts(),
                             ride_status.total_kj());
    }

    const unsigned long process_end = micros();
    unsigned long log_end;

    if (LOG_LEVEL >= LOG_LEVEL_INFO && updated_ride_status) {
        // this call takes about 11ms in non DEBUG mode if only one print call
        serial_print_state();
    }
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        snprintf_P(logbuf, 32, PSTR("proc %luus BT %luus\n"),
                   process_end-process_start, process_end-bt_start);
        logger.print(logbuf);
        log_end = micros();
        snprintf_P(logbuf,32,PSTR("logs %luus\n"), log_end - process_end);
        logger.print(logbuf);
    }

    // Reset buffers
    memset(hu_buf, 0, HU_MSG_BUF_LEN);
    memset(bike_buf, 0, BIKE_MSG_BUF_LEN);
    running_checksum = 0;
    hu_buf_bytes = bike_buf_bytes = 0;
    digitalWrite(PIN_STATE_PROC_MSG, LOW);
    return done_with_boot;
}

void loop() {
    bool messages_available = receive_message_pair();
    if (messages_available) {
        last_time_messages_seen = millis();
        boot_sequence_complete = process_message_pair();
    }

    // During bootup, we really don't want to miss a message by handling a command,
    // so only accept commands during the first half of the 200ms inter-message
    // timing if we've started seeing the boot sequence.
    // After bootup, it's not *that* critical if we drop a message, since we'll
    // see it again in 300ms, so accept commands whenever.

    bool ok_to_process_commands = (
        boot_sequence_complete
        || last_time_messages_seen == 0     // We haven't seen start of boot sequence
        || (millis() - last_time_messages_seen < 100 && last_time_messages_seen != 0));

    if (ok_to_process_commands) {
        digitalWrite(PIN_STATE_HANDLE_CMD, HIGH);
        handle_user_command_if_available();
        digitalWrite(PIN_STATE_HANDLE_CMD, LOW);
    }
    return;
}

void serial_print_state(void) {
    ride_status.serial_status_text();
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        resistance_lut.serial_status_text();
        power_service.serial_status_text();
    }
}

void handle_user_command_if_available() {
    const uint8_t buflen = 32;
    char cmdbuf[buflen];
    bool command_available = (read_BLE_command(cmdbuf, buflen) ||
                              read_serial_command(cmdbuf, buflen));
    if (command_available) run_command(cmdbuf);
}

bool read_BLE_command(char *cmdbuf, const uint8_t buflen) {
    const unsigned long prev_timeout = ble.getTimeout();
    char* res = NULL;
    int rxlen;

    // Wait no more than 3ms for data to come in.
    // At 1ms, reception is unreliable. Allow a little buffer.
    // (For the same reason, available() on the BLEUart isn't great.)
    ble.setTimeout(3);

    while (NULL == res) {
        rxlen = ble.readBLEUart(cmdbuf, buflen);
        if (rxlen == 0) break;
        res = memchr(cmdbuf, '\n', buflen);
    }
    // Restore previous timeout setting
    ble.setTimeout(prev_timeout);
    if (rxlen == 0 && NULL == res) return false;
    // Null terminate at newline
    *res = '\0';
    logger.println(res);
    return true;
}

bool read_serial_command(char *cmdbuf, const uint8_t buflen) {
    char* res = NULL;
    uint8_t available_bytes = Serial.available();
    if (available_bytes == 0) return false;

    while (NULL == res) {
        int rxlen = Serial.readBytes(cmdbuf, MIN(available_bytes, buflen));
        if (rxlen == 0) return false;
        res = memchr(cmdbuf, '\n', buflen);
    }
    // Null terminate at newline
    *res = '\0';
    return true;
}

void run_command(const char* const cmdbuf) {
    const uint8_t prev_log_level = LOG_LEVEL;
    // Command handlers
    //  LIST COMMANDS
    if (strncmp_P(cmdbuf,PSTR("help"),4) == 0) {
        logger.print(F(
            "COMMANDS:\n"
            "\thelp\tlist commands\n"
            "\tsim\treboot to simulator\n"
            "\treboot\treboot\n"
            "\tfreset\tfactory reset\n"
            "\tnolog\tdisable logging\n"
            "\tinfo\tlog level INFO\n"
            "\tdebug\t log level DEBUG\n"
            "\trlut\tdump resistance LUT\n"
            "\tble\tdump BLE module state\n"
            "\tride\tdump ride state\n"
            #ifdef ENABLE_RINGBUF
            "\tring\tdump bootup ring buffer\n"
            #endif
        ));
    }
    //  SWITCH TO SIMULATOR
    else if (strncmp_P(cmdbuf,PSTR("sim"),3) == 0) {
        logger.println(F("Rebooting to sim..."));
        EEPROM.write(EEPROM_FORCE_SIMULATION_AT_STARTUP, true);
        reboot();
    }
    else if (strncmp_P(cmdbuf, PSTR("freset"), 6) == 0) {
        for (int i=0; i < EEPROM_MAX_ADDRESS; EEPROM.update(i++,0));
        ble.factoryReset();
        reboot();
    }
    else if (strncmp_P(cmdbuf, PSTR("reboot"), 6) == 0) {
        reboot();
    }
    //  SWITCH LOGGING MODES
    else if (strncmp_P(cmdbuf,PSTR("info"),4) == 0) {
        logger.println(F("Logs->INFO"));
        LOG_LEVEL = LOG_LEVEL_INFO;
    } else if (strncmp_P(cmdbuf,PSTR("debug"),5) == 0) {
        logger.println(F("Logs->DEBUG"));
        LOG_LEVEL = LOG_LEVEL_DEBUG;
    } else if (strncmp_P(cmdbuf,PSTR("nolog"),5) == 0) {
        logger.println(F("Logs->NONE"));
        LOG_LEVEL = LOG_LEVEL_NONE;
    }
    //  PRINT CURRENT STATE
    else if (strncmp_P(cmdbuf, PSTR("rlut"), 4) == 0) {
        LOG_LEVEL = LOG_LEVEL_MAX;
        resistance_lut.serial_status_text();
        LOG_LEVEL = prev_log_level;
    } else if (strncmp_P(cmdbuf, PSTR("ble"), 3) == 0) {
        LOG_LEVEL = LOG_LEVEL_MAX;
        power_service.serial_status_text();
        LOG_LEVEL = prev_log_level;
    } else if (strncmp_P(cmdbuf, PSTR("ride"), 4) == 0) {
        LOG_LEVEL = LOG_LEVEL_MAX;
        ride_status.serial_status_text();
        LOG_LEVEL = prev_log_level;
    }
    #ifdef ENABLE_RINGBUF
    else if (strncmp_P(cmdbuf, PSTR("ring"), 4) == 0) {
            dump_ringbuf();
    }
    #endif

    return;
}

void serial_log_messagepair_text(void) {
    const int buf_len = 16 + 3 + BIKE_MSG_BUF_LEN * 3 + 1;
    char buf[16 + 3 + BIKE_MSG_BUF_LEN * 3 + 1];
    uint8_t base = 0;
    uint8_t len;
    len = snprintf_P(buf + base, buf_len - base,
                     PSTR("\n\t%02hhX %02hhX %02hhX %02hhX\n%d\t"),
                     hu_buf[0], hu_buf[1], hu_buf[2], hu_buf[3], bike_buf_bytes);
    base = MIN(buf_len, base + len);
    for (uint8_t i = 0; i < bike_buf_bytes; i++) {
        snprintf_P(buf + base, buf_len - base, PSTR("%02hhX "), bike_buf[i]);
        base = MIN(buf_len, base + 3);
    }
    if (base < buf_len - 1) {
        buf[base] = '\n';
        buf[base + 1] = '\0';
    }
    logger.print(buf);
}
