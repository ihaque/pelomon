/* Software Peloton emulator and decoding classes
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef _PELOTON_H_
#define _PELOTON_H_
#ifndef PIN_RX_FROM_HU
#define PIN_RX_FROM_HU 11
#define PIN_TX_TO_HU 3
#define PIN_RX_FROM_BIKE 10
#define PIN_TX_TO_BIKE 2
#define INVERT_PELOTON_SERIAL false

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL LOG_LEVEL_NONE

#define SIMULATOR_MESSAGE_INTERVAL_MILLIS 100
#endif

#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

bool message_is_valid(uint8_t* msg, uint8_t len);
bool message_is_valid(uint8_t* msg, uint8_t len) {
    // Peloton messages always end in F6
    if (msg[len-1] != 0xF6) {
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
            Serial.println(F("Invalid terminator"));
        return false;
    }

    // First byte is always F5 / FE / F7 (head unit) or F1 (bike)
    // F5 and F1 are the ones used during rides and are frequent so
    // put them first in check.
    if (! (msg[0] == 0xF5 || msg[0] == 0xF1 ||
           msg[0] == 0xF7 || msg[0] == 0xFE)) {
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) Serial.println(F("Invalid header"));
        return false;
    }

    // Verify length
    if (msg[0] == 0xF1) {
        if (msg[2] + 5 != len) {
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) Serial.println(F("Invalid length F1"));
            return false;
        }
    } else if (msg[0] == 0xF5 || msg[0] == 0xFE || msg[0] == 0xF7) {
        if (len != 4) {
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
                Serial.print(F("Invalid length HU "));
                Serial.println(len);
            } 
            return false;
        }
    }

    // Verify checksum
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len-2; checksum += msg[i++]);
    if (checksum != msg[len-2]) return false;

    return true;
}


class PelotonSimulator;
enum HUPacketType {
    STARTUP_UNKNOWN = 0xFE,
    READ_RESISTANCE_TABLE = 0xF7,
    READ_FROM_BIKE = 0xF5
};
enum Requests {
               READ_RESISTANCE_TABLE_00 = 0, READ_RESISTANCE_TABLE_01,
               READ_RESISTANCE_TABLE_02, READ_RESISTANCE_TABLE_03,
               READ_RESISTANCE_TABLE_04, READ_RESISTANCE_TABLE_05,
               READ_RESISTANCE_TABLE_06, READ_RESISTANCE_TABLE_07,
               READ_RESISTANCE_TABLE_08, READ_RESISTANCE_TABLE_09,
               READ_RESISTANCE_TABLE_0A, READ_RESISTANCE_TABLE_0B,
               READ_RESISTANCE_TABLE_0C, READ_RESISTANCE_TABLE_0D,
               READ_RESISTANCE_TABLE_0E, READ_RESISTANCE_TABLE_0F,
               READ_RESISTANCE_TABLE_10, READ_RESISTANCE_TABLE_11,
               READ_RESISTANCE_TABLE_12, READ_RESISTANCE_TABLE_13,
               READ_RESISTANCE_TABLE_14, READ_RESISTANCE_TABLE_15,
               READ_RESISTANCE_TABLE_16, READ_RESISTANCE_TABLE_17,
               READ_RESISTANCE_TABLE_18, READ_RESISTANCE_TABLE_19,
               READ_RESISTANCE_TABLE_1A, READ_RESISTANCE_TABLE_1B,
               READ_RESISTANCE_TABLE_1C, READ_RESISTANCE_TABLE_1D,
               READ_RESISTANCE_TABLE_1E,
               RPM = 0x41,
               POWER = 0x44,
               RESISTANCE = 0x4A,
               RESISTANCE_TABLE_RESPONSE = 0xF7,
               BIKE_ID = 0xFB,
               UNKNOWN_INIT_REQUEST = 0xFE};
class BikeMessage {
    public:
    Requests request;
    uint16_t value;
    bool is_valid;
    // Only parse valid messages _from the bike_, not the HU
    BikeMessage(uint8_t* bike_msg, const uint8_t len) {
        request = 0;
        value = 0;
        if (!message_is_valid(bike_msg, len) || bike_msg[0] != 0xF1) {
            is_valid = false;
            return;
        }
        request = bike_msg[1];
        if (request == BIKE_ID) {
            is_valid = true;
            value = 0;
            return;
        }
        const uint8_t payload_length = bike_msg[2];
        for (uint8_t i = 2 + payload_length; i > 2; i--) {
            // -30 = Convert from ASCII to numeric
            uint8_t next_digit = bike_msg[i] - 0x30;
            // Check for overflow
            if (value > 6553 || (value == 6553 && next_digit > 5)) {
                is_valid = false;
                return;
            }
            value = value * 10 + next_digit;
        }
        is_valid = true;
        return;
    }
    uint8_t encode(uint8_t* buffer, const uint8_t buffer_len) {
        // To be implemented
        return 0;
    }
};
class HUMessage {
    public:
    HUPacketType packet_type;
    Requests request;
    bool is_valid;
    HUMessage(uint8_t* hu_msg, const uint8_t len) {
        if (!message_is_valid(hu_msg, len) ||
            !(hu_msg[0] == 0xF5 || hu_msg[0] == 0xF7 || hu_msg[0] == 0xFE)) {
            is_valid = false;
            return;
        }
        packet_type = hu_msg[0];
        request = hu_msg[1];
        is_valid = true;
    }
};
class SimulatedSerial {
    private:
        uint8_t buf[15];
        uint8_t len;
        uint8_t loc;
        uint8_t id;
        PelotonSimulator* simulator;
    public:
    SimulatedSerial(const uint8_t id_, PelotonSimulator* psim): id(id_), simulator(psim) {
    }
    void begin(const int rate) {
        len = loc = 0;
        return;
    }
    void listen();
    int8_t available() {
        if (loc < len) return len - loc;
        else return 0;
    }
    uint8_t read() {
        if (!available()) return 0xFF;
        else return buf[loc++];
    }
    void push(const uint8_t* msg, uint8_t nbytes) {
        if (nbytes > 15) nbytes=15;
        memcpy(buf, msg, nbytes);
        loc = 0;
        len = nbytes;
    }
};

// Real-ish values. Delta encoded to save space rather than
// using 16 bits for each one.
uint8_t const RESISTANCE_LUT_DELTA_ENCODED[] PROGMEM = {
    164,  5, 17, 36, 75, 72, 71, 57,
     61, 51, 44, 34, 39, 31, 26, 20,
     24, 18, 16, 13, 15, 12, 10, 10,
      9,  8,  6,  8,  6,  5,  4};

class PelotonSimulator {
    public:
    SimulatedSerial hu, bike;
    uint8_t next_message_to_send;
    uint32_t last_hu_timestamp;

    PelotonSimulator(): hu(0, this), bike(1, this),
                        next_message_to_send(UNKNOWN_INIT_REQUEST),
                        last_hu_timestamp(0) {
    };
    void updateState(const uint8_t bike_listening) {
        uint8_t msg[15];
        uint32_t current_time = millis();
        char logbuf[32];
        if (!bike_listening) {
            // State machine expects messages from HU
            // If HU already has a message in buffer then don't push another.
            if (hu.available()) return;

            // Only query the "bike" every 100ms in steady state
            bool send_message = true;
            if (next_message_to_send == RPM || next_message_to_send == POWER ||
                next_message_to_send == RESISTANCE) {
                send_message = (current_time - last_hu_timestamp >=
                                SIMULATOR_MESSAGE_INTERVAL_MILLIS);
            }
            if (!send_message) return;

            if (next_message_to_send == UNKNOWN_INIT_REQUEST) {
                msg[0] = 0xFE;
                msg[1] = 0x00;
            } else if (next_message_to_send >= READ_RESISTANCE_TABLE_00 &&
                       next_message_to_send <= READ_RESISTANCE_TABLE_1E) {
                msg[0] = 0xF7;
                msg[1] = next_message_to_send;
            } else {
                msg[0] = 0xF5;
                msg[1] = next_message_to_send;
            }

            msg[2] = msg[0] + msg[1];
            msg[3] = 0xF6;

            if (0) {
                Serial.print(F("Simulator pushing to HU "));
                snprintf_P(logbuf, 32, PSTR("%hhx %hhx %hhx %hhx"),
                         msg[0], msg[1], msg[2], msg[3]);
                Serial.println(logbuf);
            }
            hu.push(msg, 4);
            last_hu_timestamp = current_time;
        } else {
            // State machine expects message from bike
            // If bike already has a message in buffer don't push another
            if (bike.available()) return;

            // Bike never needs to delay in its responses
            msg[0] = 0xF1;
            msg[1] = next_message_to_send;
            if (next_message_to_send == UNKNOWN_INIT_REQUEST) {
                msg[2] = 3;
                msg[3] = msg[4] = msg[5] = 0x30;
                next_message_to_send = BIKE_ID;
            } else if (next_message_to_send == BIKE_ID) {
                msg[2] = 7;
                for (uint8_t i = 0; i < 7; i++) msg[3+i] = 0x30;
                next_message_to_send = READ_RESISTANCE_TABLE_00;
            } else if (next_message_to_send >= READ_RESISTANCE_TABLE_00 &&
                       next_message_to_send <= READ_RESISTANCE_TABLE_1E) {
                uint16_t resistance = 0;
                for (uint8_t i = READ_RESISTANCE_TABLE_00; i <= next_message_to_send; i++) {
                    resistance += (uint8_t) pgm_read_byte(RESISTANCE_LUT_DELTA_ENCODED +
                                                          (i - READ_RESISTANCE_TABLE_00));
                }
                msg[1] = 0xF7;
                msg[2] = 4;
                msg[3] = 0x30 + resistance % 10;
                msg[4] = 0x30 + (resistance / 10) % 10;
                msg[5] = 0x30 + resistance / 100;
                msg[6] = 0x30;
                if (next_message_to_send == READ_RESISTANCE_TABLE_1E) {
                    next_message_to_send = RPM;
                } else {
                    next_message_to_send++;
                }
            } else if (next_message_to_send == RPM) {
                uint8_t rpm = 75 + current_time % 10;
                msg[2] = 3;
                msg[3] = 0x30 + rpm % 10;
                msg[4] = 0x30 + rpm / 10;
                msg[5] = 0x30;
                next_message_to_send = POWER;
            } else if (next_message_to_send == POWER) {
                uint8_t power_millis = 150 + current_time % 10;
                msg[1] = next_message_to_send;
                msg[2] = 5;
                msg[3] = 0x30;
                msg[4] = 0x30 + power_millis % 10;
                msg[5] = 0x30 + (power_millis / 10) % 10;
                msg[6] = 0x30 + power_millis / 100;
                msg[7] = 0x30;
                next_message_to_send = RESISTANCE;
            } else if (next_message_to_send == RESISTANCE) {
                msg[1] = next_message_to_send;
                msg[2] = 4;
                msg[3] = 0x30;
                msg[4] = 0x36;
                msg[5] = 0x35;
                msg[6] = 0x34;
                next_message_to_send = RPM;
            }
            // Compute checksum and add terminating byte
            uint8_t checksum = 0;
            for (uint8_t i=0; i < msg[2]+3; checksum += msg[i++]);
            msg[msg[2] + 3] = checksum;
            msg[msg[2] + 4] = 0xF6;
            bike.push(msg, msg[2] + 5);
            if (0) {
                Serial.print(F("Simulator pushing to bike "));
                snprintf_P(logbuf, 32,
                           PSTR("%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx "),
                         msg[0], msg[1], msg[2], msg[3],
                         msg[4], msg[5], msg[6], msg[7]);
                Serial.print(logbuf);
                if (msg[2] > 3) {
                    snprintf_P(logbuf, 32, PSTR("%hhx "), msg[8]);
                    Serial.print(logbuf);
                }
                if (msg[2] > 4) {
                    snprintf_P(logbuf, 32, PSTR("%hhx "), msg[9]);
                    Serial.print(logbuf);
                }
                Serial.println(" ");
            }
        }
    }
};
void SimulatedSerial::listen() {
   simulator->updateState(id);
   return;
}
class PelotonProxy {
    private:
    PelotonSimulator simulator;
    SoftwareSerial hw_hu;
    SoftwareSerial hw_bike;
    bool use_simulator;

    public:
    PelotonProxy() : hw_hu(PIN_RX_FROM_HU, PIN_TX_TO_HU,
                             INVERT_PELOTON_SERIAL),
                       hw_bike(PIN_RX_FROM_BIKE, PIN_TX_TO_BIKE,
                               INVERT_PELOTON_SERIAL)
    {}
    void initialize(bool select_simulator) {
        use_simulator = select_simulator;
        hw_bike.begin(19200);
        hw_hu.begin(19200);
    }
    void hu_listen() {
        digitalWrite(PIN_STATE_LISTEN_HU, HIGH);
        digitalWrite(PIN_STATE_LISTEN_BIKE, LOW);
        if (use_simulator) simulator.hu.listen();
        else hw_hu.listen();
    }
    void bike_listen() {
        digitalWrite(PIN_STATE_LISTEN_HU, LOW);
        digitalWrite(PIN_STATE_LISTEN_BIKE, HIGH);
        if (use_simulator) simulator.bike.listen();
        else hw_bike.listen();
    }
    int8_t hu_available() {
        if (use_simulator) return simulator.hu.available();
        else return hw_hu.available();
    }
    int8_t bike_available() {
        if (use_simulator) return simulator.bike.available();
        else return hw_bike.available();
    }
    uint8_t hu_read() {
        if (use_simulator) return simulator.hu.read();
        else return hw_hu.read();
    }
    uint8_t bike_read() {
        if (use_simulator) return simulator.bike.read();
        else return hw_bike.read();
    }
};
#endif

