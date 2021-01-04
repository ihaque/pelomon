/* Hardware Peloton emulator
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 *
 * Usage: Upload to Arduino Uno or compatible. Keep A0 and A5 connected
 *        to GND during power on until you see a slow blinking and PeloMon
 *        is initialized. Disconnect A5 to send bike bootup sequence. When
 *        blink switches to pattern of 3 rapid blinks, bootup sequence is
 *        complete. Disconnect A0 to start sending message pattern from a
 *        ride. Reconnect A0 to GND to pause ride.
 *        To re-enter bootup sequence, must reset board.
 * 
 * Note: The Arduino Uno outputs 5V and must be connected through a level
 *       shifter if communicating with a 3.3V device. The PeloMon uses an
 *       inverting level shifter between the Peloton and the board; this
 *       device is designed to signal to the inverter/level shifter and
 *       therefore uses inverted serial communication, emulating the logical
 *       levels seen on the Peloton wires. Note that the physical voltage
 *       levels will not be true to the Peloton as there is no true UART
 *       driving negative voltage.
 */

#include <Arduino.h>
#include <SoftwareSerial.h>

// Arbitrary, but corresponds to the HU_RX and BIKE_RX pins on the PeloMon.
#define PIN_HU_TX 11
#define PIN_BIKE_TX 10

// Unused - we don't receive in this sketch
#define PIN_HU_RX_PLACEHOLDER 9
#define PIN_BIKE_RX_PLACEHOLDER 8

/* The Uno transmits fuzz on its digital pins during init/reset, which
 * confuses the PeloMon (and is unlike what the Peloton actually
 * transmits). To get around this, we hold a pin low at start and release
 * it when we want the Peloton init sequence to transmit. This allows
 * bringing up the Uno and pausing, then resetting the PeloMon, then
 * releasing the Uno to start the emulation.
 */
// If this pin is low, we will pause in initialization
#define PIN_PAUSE_INIT A5
// If this pin is low, we will stop sending ride messages.
#define PIN_PAUSE_RIDE A0

// If emulating signal as it comes over the wire from the Peloton,
// leave INVERTED_SERIAL set to true. If connecting hardware emulator
// directly to PeloMon (after the inverter/level shifter), set to false.
#define INVERTED_SERIAL true


/* Global state
 *
 * `hu`, `bike`: Serial interfaces for emulated HU and bike communications
 *
 * `next_message_time`: deadline timestamp (in millis()) when the "HU" should
 *                      send out its next message
 * 
 * `next_message`: only relevant once we have entered the "ride" portion in loop().
 *                 The type of the next ride message (0x41, 0x44, 0x4A) that should
 *                 sent out.
 *
 * `led_state`: The LED is kept high during the "bootup" sequence and toggled for
 *              each ride message. Tracks that state.
 */
SoftwareSerial hu(PIN_HU_RX_PLACEHOLDER, PIN_HU_TX,
                  INVERTED_SERIAL);
SoftwareSerial bike(PIN_BIKE_RX_PLACEHOLDER, PIN_BIKE_TX,
                    INVERTED_SERIAL);
unsigned long next_message_time;
uint8_t next_message;
bool led_state;

/* Synchronously writes a Peloton message on the given interface.
 * 
 * `buf`: The message. *Must not* have checksum and terminating 0xF6 byte; this routine
 *        will add them.
 *
 * `buf_len`: Length of buffer. Must not include two bytes at end for checksum/footer.
 *
 * Returns the total number of bytes sent.
 */
uint8_t write_packet(SoftwareSerial& interface, const uint8_t* buf, const uint8_t len) {
    /* Adds checksum and terminating byte */
    uint8_t footer[2] = {0, 0xF6};
    uint8_t bytes_sent;

    // Compute checksum byte
    for (uint8_t i=0; i < len; footer[0] += buf[i++]);

    bytes_sent = interface.write(buf, len);
    bytes_sent += interface.write(footer, 2);
    interface.flush();
    return bytes_sent;
}

/* Delay until the given timestamp (in millis()) has passed.
 */
unsigned long wait_until(const unsigned long target_time) {
    unsigned long current_time = millis();
    while (current_time < target_time) {
        delay(target_time - current_time);
        current_time = millis();
    }
    return current_time;
}

/* Writes out a pair of (HU message, bike message) at a given time.
 *
 * Message buffers *must not* have the checksum and footer 0xF6 bytes; this
 * routine will compute them for you.
 * 
 * `bike_response_latency` param indicates how much "latency" in microseconds
 * to emulate in the bike's response to the HU.
 *
 * `target_time` param indicates the millis() timestamp at or after which to
 * send the message. If the deadline has already passed, it will send
 * immediately (i.e., timing is best effort)
 * 
 * Returns the millisecond timestamp when the HU write started.
 */
unsigned long write_pair_at(const uint8_t* hu_msg, const uint8_t hu_len,
                            const uint8_t* bike_msg, const uint8_t bike_len,
                            const uint16_t bike_response_latency,
                            const unsigned long target_time) {
    unsigned long started_write_at = wait_until(target_time);
    write_packet(hu, hu_msg, hu_len);
    delayMicroseconds(bike_response_latency);
    write_packet(bike, bike_msg, bike_len);
    return started_write_at;
}

void setup() {
    pinMode(PIN_HU_TX, OUTPUT);
    pinMode(PIN_BIKE_TX, OUTPUT);
    pinMode(PIN_HU_RX_PLACEHOLDER, INPUT);
    pinMode(PIN_BIKE_RX_PLACEHOLDER, INPUT);
    pinMode(PIN_PAUSE_RIDE, INPUT_PULLUP);
    pinMode(PIN_PAUSE_INIT, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    hu.begin(19200);
    bike.begin(19200);

    // LED will be kept high during bootup sequence.
    digitalWrite(LED_BUILTIN, HIGH);

    // Hold here on initialization until released
    while (digitalRead(PIN_PAUSE_INIT) == LOW) {
        digitalWrite(LED_BUILTIN, HIGH); delay(150);
        digitalWrite(LED_BUILTIN, LOW);  delay(250);
        digitalWrite(LED_BUILTIN, HIGH); delay(150);
        digitalWrite(LED_BUILTIN, LOW);  delay(250);
        digitalWrite(LED_BUILTIN, HIGH); delay(150);
        digitalWrite(LED_BUILTIN, LOW);  delay(750);
    }

    // Send messages that are sent once on bike bootup
    uint16_t delay_us;
    unsigned long sent_at;
    uint8_t hu_len, bike_len;
    uint8_t hu_msg[2], bike_msg[16];
    // Boot time messages come every 200ms.
    const uint8_t inter_boot_delay_ms = 200;

    const uint8_t hu_init_msg[] = {2,
                                   0xFE, 0x00};
    const uint8_t bike_init_msg[] = {6,
                                     0xF1, 0xFE,
                                     0x03, 0x35, 0x31, 0x30};
    const uint8_t hu_bikeid_msg[] = {2,
                                     0xF5, 0xFB};
    const uint8_t bike_bikeid_msg[] = {10,
                                       0xF1, 0xFB,
                                       0x01, 0x13, 0x09, 0x12,
                                       0x34, 0x56, 0x78};
    uint8_t const RESISTANCE_LUT_DELTA_ENCODED[] = {
        164,  5, 17, 36, 75, 72, 71, 57,
         61, 51, 44, 34, 39, 31, 26, 20,
         24, 18, 16, 13, 15, 12, 10, 10,
          9,  8,  6,  8,  6,  5,  4};

    // Init 0 message
    // Latency from HU end to Bike start on bootup messages 200-1000us
    delay_us = (uint16_t) random(200, 1000);
    sent_at = write_pair_at(hu_init_msg+1, hu_init_msg[0],
                            bike_init_msg+1, bike_init_msg[0],
                            delay_us, 0);
    next_message_time = sent_at + inter_boot_delay_ms;

    // Bike ID message
    delay_us = (uint16_t) random(200, 1000);
    sent_at = write_pair_at(hu_bikeid_msg+1, hu_bikeid_msg[0],
                            bike_bikeid_msg+1, bike_bikeid_msg[0],
                            delay_us, next_message_time);
    next_message_time = sent_at + inter_boot_delay_ms;

    bike_msg[0] = 0xF1; bike_msg[1] = 0xF7; bike_msg[2] = 0x04;
    hu_msg[0] = 0xF7; hu_msg[1] = 0;
    uint16_t resistance = 0;
    for (uint8_t res_index=0; res_index <= 0x1E; res_index++) {
        hu_msg[1] = res_index;
        resistance += RESISTANCE_LUT_DELTA_ENCODED[res_index];
        bike_msg[3] = 0x30 + resistance % 10;
        bike_msg[4] = 0x30 + (resistance / 10) % 10;
        bike_msg[5] = 0x30 + (resistance / 100) % 10;
        bike_msg[6] = 0x30 + (resistance / 1000) % 10;
        delay_us = (uint16_t) random(200, 1000);
        sent_at = write_pair_at(hu_msg, 2, bike_msg, 7, delay_us,
                                next_message_time);
        next_message_time = sent_at + inter_boot_delay_ms;
    }

    next_message = 0x41;
    digitalWrite(LED_BUILTIN, LOW);
    led_state = false;
}

void loop() {
    // Send the messages observed during a ride
    const unsigned long current_time = millis();
    // Ride-time messages come every 100ms.
    const uint8_t inter_ride_delay_ms = 100;
    unsigned long sent_at;
    uint16_t delay_us;
    uint8_t hu_msg[2];
    uint8_t bike_msg[8];

    hu_msg[0] = 0xF5; hu_msg[1] = next_message;
    bike_msg[0] = 0xF1; bike_msg[1] = next_message;

    // Toggle LED on every message
    if (led_state) {
        led_state = false;
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        led_state = true;
        digitalWrite(LED_BUILTIN, HIGH);
    }

    if (digitalRead(PIN_PAUSE_RIDE) == LOW) {
        // If we're pausing the ride, reset state to start
        // a new "ride" as soon as this pin goes high.
        next_message = 0x41;
        next_message_time = 0;
        // Make the blinks slower when we're paused.
        delay(500);
        return;
    }

    // Latency on
    //  41: 2307, 538, 1794
    //  4A: 1614, 326, 866
    //  44: 867, 615, 546
    if (next_message == 0x41) {
        // Cadence
        bike_msg[2] = 3;
        uint8_t rpm = 75 + current_time % 10;
        bike_msg[3] = 0x30 + rpm % 10;
        bike_msg[4] = 0x30 + rpm / 10;
        bike_msg[5] = 0x30;
        delay_us = (uint16_t) random(200, 2700);
        next_message = 0x44;
    } else if (next_message == 0x44) {
        // Output
        bike_msg[2] = 5;
        uint8_t power = 150 + current_time % 10;
        bike_msg[3] = 0x30;
        bike_msg[4] = 0x30 + power % 10;
        bike_msg[5] = 0x30 + (power / 10) % 10;
        bike_msg[6] = 0x30 + power / 100;
        bike_msg[7] = 0x30;
        delay_us = (uint16_t) random(200, 2700);
        next_message = 0x4A;
    } else if (next_message == 0x4A) {
        // Raw resistance
        bike_msg[2] = 4;
        bike_msg[3] = 0x30;
        bike_msg[4] = 0x36;
        bike_msg[5] = 0x35;
        bike_msg[6] = 0x34;
        delay_us = (uint16_t) random(200, 2700);
        next_message = 0x41;
    }
    // Send ride messages
    sent_at = write_pair_at(hu_msg, 2, bike_msg, bike_msg[2]+3, delay_us,
                            next_message_time);
    next_message_time = sent_at + inter_ride_delay_ms;
}
