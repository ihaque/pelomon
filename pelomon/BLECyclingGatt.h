/* Implementation of Bluetooth LE Cycling Power and Cycling Speed and Cadence
 * Services for the Adafruit Bluefruit LE.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef BLE_CYCLING_GATT_H
#define BLE_CYCLING_GATT_H
#include <EEPROM.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BLEGatt.h"
#include "ble_constants.h"
#include "eeprom_map.h"

#define APPEND_BUFFER(buffer,base,field) \
    memcpy(buffer+base,&field,sizeof(field)); \
    base += sizeof(field);


uint16_t fletcher16(const char* str, uint16_t len) {
    uint8_t lo, hi;
    lo = hi = 0;
    for (; len > 0; len--, str++) {
        lo += *str;
        hi += lo;
    }
    uint16_t rv = hi;
    rv <<= 8;
    rv |= lo;
    return rv;
}

// These strings don't end up referenced anywhere so the compiler removes them.
const char line_1[] PROGMEM = "ID=01,UUID=0x1818";
const char line_2[] PROGMEM = "  ID=01,UUID=0x2A65,PROPERTIES=0x02,MIN_LEN=4,MAX_LEN=4,DATATYPE=0,VALUE=0";
const char line_3[] PROGMEM = "  ID=02,UUID=0x2A63,PROPERTIES=0x10,MIN_LEN=6,MAX_LEN=6,DATATYPE=0,VALUE=00-00-00-00-00-00";
const char line_4[] PROGMEM = "  ID=03,UUID=0x2A5D,PROPERTIES=0x02,MIN_LEN=1,MAX_LEN=1,DATATYPE=0,VALUE=0";
const char line_5[] PROGMEM = "ID=02,UUID=0x1816";
const char line_6[] PROGMEM = "  ID=04,UUID=0x2A5C,PROPERTIES=0x02,MIN_LEN=2,MAX_LEN=2,DATATYPE=0,VALUE=0";
const char line_7[] PROGMEM = "  ID=05,UUID=0x2A5B,PROPERTIES=0x10,MIN_LEN=11,MAX_LEN=11,DATATYPE=0,VALUE=00-00-00-00-00-00-00-00-00-00-00";
const char line_8[] PROGMEM = "  ID=06,UUID=0x2A5D,PROPERTIES=0x02,MIN_LEN=1,MAX_LEN=1,DATATYPE=0,VALUE=0";
const char line_9[] PROGMEM = "  ID=07,UUID=0x2A55,PROPERTIES=0x28,MIN_LEN=1,MAX_LEN=5,DATATYPE=0,VALUE=0";
const char line_10[] PROGMEM = "OK";

const char* const EXPECTED_GATT_DEFNS[] PROGMEM = {line_1, line_2, line_3,
                                                   line_4, line_5, line_6, line_7,
                                                   line_8, line_9, line_10};


// Computed from above strings using following Python
/*
    def fletcher16(s):
        lo, hi = 0,0
        for c in s:
            lo = (lo + ord(c)) & 0xFF
            hi = (hi + lo) & 0xFF
        return (hi << 8) | lo
*/
uint16_t const EXPECTED_GATT_DEFNS_FLETCHER16[] PROGMEM = {
    0xAC45, //  "ID=01,UUID=0x1818"
    0x389C, //  "  ID=01,UUID=0x2A65,PROPERTIES=0x02,MIN_LEN=4,MAX_LEN=4,DATATYPE=0,VALUE=0"
    0x228F, //  "  ID=02,UUID=0x2A63,PROPERTIES=0x10,MIN_LEN=6,MAX_LEN=6,DATATYPE=0,VALUE=00-00-00-00-00-00"
    0x39A6, //  "  ID=03,UUID=0x2A5D,PROPERTIES=0x02,MIN_LEN=1,MAX_LEN=1,DATATYPE=0,VALUE=0"
    0xB744, //  "ID=02,UUID=0x1816"
    0x77A8, //  "  ID=04,UUID=0x2A5C,PROPERTIES=0x02,MIN_LEN=2,MAX_LEN=2,DATATYPE=0,VALUE=0"
    0x4DB9, //  "  ID=05,UUID=0x2A5B,PROPERTIES=0x10,MIN_LEN=11,MAX_LEN=11,DATATYPE=0,VALUE=00-00-00-00-00-00-00-00-00-00-00"
    0x05A9, //  "  ID=06,UUID=0x2A5D,PROPERTIES=0x02,MIN_LEN=1,MAX_LEN=1,DATATYPE=0,VALUE=0"
    0x93A7, //  "  ID=07,UUID=0x2A55,PROPERTIES=0x28,MIN_LEN=1,MAX_LEN=5,DATATYPE=0,VALUE=0"
    0xE99A, //  "OK"
};
const uint8_t EXPECTED_GATT_DEFNS_LINE_COUNT = 10;


struct ProgmemComparatorState {
    bool is_equal;
    uint8_t line_number;
    uint8_t total_lines;
    void* pgm_entry_table; // char** for strings, uint16_t* for hashes
};

void fletcher16_comparator_callback(void* callback_data, char* linebuf, uint16_t line_len) {
    ProgmemComparatorState* state = (ProgmemComparatorState*) callback_data;
    if (state->line_number >= state->total_lines) return;

    uint16_t* pgm_hash_table = (uint16_t*) (state->pgm_entry_table);

    // Read the pointer to the next PROGMEM string out of PROGMEM
    const uint16_t next_pgm_hash = (uint16_t) pgm_read_word(&(pgm_hash_table[state->line_number]));

    const bool hashes_matched = next_pgm_hash == fletcher16(linebuf, line_len);

    state->is_equal = (state->is_equal && hashes_matched);
    state->line_number++;

    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        char logbuf[128];
        snprintf_P(logbuf, 32, PSTR("\tfinal matching %d"), state->is_equal ? 1 : 0);
        Serial.println(logbuf);
    }
}

void string_comparator_callback(void* callback_data, char* linebuf, uint16_t line_len) {
    ProgmemComparatorState* state = (ProgmemComparatorState*) callback_data;
    if (state->line_number >= state->total_lines) return;

    char** pgm_line_table = (char**) (state->pgm_entry_table);

    // Read the pointer to the next PROGMEM string out of PROGMEM
    const char* next_pgm_line = (char*)pgm_read_word(&(pgm_line_table[state->line_number]));

    const uint16_t next_pgm_line_len = strnlen_P(next_pgm_line, line_len+1);
    const int lines_matched = strncmp_P(linebuf, next_pgm_line, line_len);
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        char logbuf[128];
        Serial.print(F("Checking lines:\n\t"));
        Serial.println(linebuf);
        Serial.print('\t');
        strncpy_P(logbuf, next_pgm_line, 128);
        Serial.println(logbuf);
        snprintf_P(logbuf, 32, PSTR("\tlengths: %d vs %d"), line_len, next_pgm_line_len);
        Serial.println(logbuf);
        snprintf_P(logbuf, 32, PSTR("\tstrcmp: %d"), lines_matched);
        Serial.println(logbuf);
        snprintf_P(logbuf, 32, PSTR("\tinitial matching %d"), state->is_equal ? 1 : 0);
        Serial.println(logbuf);
    }
    state->is_equal = (state->is_equal
                       && (line_len == next_pgm_line_len)
                       && (0 == lines_matched));
    state->line_number++;
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        char logbuf[128];
        snprintf_P(logbuf, 32, PSTR("\tfinal matching %d"), state->is_equal ? 1 : 0);
        Serial.println(logbuf);
    }
}

void logging_callback(void* callback_data, char* linebuf, uint16_t line_len) {
    char logbuf[32];
    snprintf_P(logbuf, 32,PSTR("LOG CALLBACK: %d\n\""),line_len);
    Serial.print(logbuf);
    Serial.print(linebuf);
    Serial.println("\"");
}


class BLECyclingPower {
    // Exposes both the Cycling Power and the Cycling Speed and Cadence
    // Features
    private:
    Adafruit_BLE& ble_;
    Adafruit_BLEGatt gatt_;
    Logger& logger;
    uint8_t cp_service_id;
    uint8_t cp_feature_id;
    uint8_t cp_measurement_id;
    uint8_t cp_sensor_location_id;

    uint8_t csc_service_id;
    uint8_t csc_feature_id;
    uint8_t csc_measurement_id;
    uint8_t csc_sensor_location_id;
    uint8_t sc_control_point_id;

    public:
    BLECyclingPower(Adafruit_BLE& ble, Logger& logger_): ble_(ble), gatt_(ble), logger(logger_) {};

    void initialize() {
        // If we haven't set up the module and GATTs/characteristics, do so.
        load_or_setup_gatts();
        
        // Software reset module on bringup
        ble_.reset();

        // Set up advertising data and name
        ble_.sendCommandCheckOK(F("AT+GAPDEVNAME=PeloMon"));
        /* Advertising data:
        https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
        https://github.com/sputnikdev/bluetooth-gatt-parser/blob/master/src/main/resources/
                gatt/characteristic/org.bluetooth.characteristic.gap.appearance.xml
            02 01 06:          Flags -- LE General Discoverable, BR/EDR Not Supported
            02 0A 00:          Tx power -- 0 dBm
            11 06 9E...6E      128-bit service UUID 6E...9E (UART SERVICE)
            05 02 18 18 16 18  16-bit service UUIDs
                                 0x1818 (CYCLING POWER SERVICE)
                                 0x1816 (CYCLING SPEED/CADENCE SERVICE)
        */
        ble_.sendCommandCheckOK(
            F("AT+GAPSETADVDATA="
              "02-01-06-"
              "02-0a-00-"
              "11-06-9e-ca-dc-24-0e-e5-a9-e0-93-f3-a3-b5-01-00-40-6e-"
              "05-02-18-18-16-18"
              ));
        ble_.reset();

        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            logger.print(F("Checking GATTs\n"));
            ble_.sendCommandCheckOK(F("AT+GATTLIST"));
        }
        // Set up initial values for feature and sensor location

        gatt_.setChar(cp_sensor_location_id, SENSOR_LOCATION_LEFT_CRANK);
        gatt_.setChar(csc_sensor_location_id, SENSOR_LOCATION_LEFT_CRANK);

        gatt_.setChar(cp_feature_id,
                      (uint32_t) (CPF_CRANK_REVOLUTION_DATA_SUPPORTED |
                                  CPF_WHEEL_REVOLUTION_DATA_SUPPORTED |
                                  CPF_ACCUMULATED_ENERGY_SUPPORTED));
        gatt_.setChar(csc_feature_id,
                      (uint16_t) (CSCF_CRANK_REVOLUTION_DATA_SUPPORTED |
                                  CSCF_WHEEL_REVOLUTION_DATA_SUPPORTED));
        const uint8_t zero = 0;
        gatt_.setChar(sc_control_point_id, &zero, 1);
        return;
    }

    bool gatts_as_expected() {
        // NB: this function must be updated if gatt setup is changed
        char linebuf[128];

        // Set up a comparator to be called on a line-by-line basis.
        ProgmemComparatorState comparator;
        comparator.is_equal = true;
        comparator.line_number = 0;
        comparator.total_lines = EXPECTED_GATT_DEFNS_LINE_COUNT;
        comparator.pgm_entry_table = (void*) EXPECTED_GATT_DEFNS_FLETCHER16;

        // Allow 100ms between sending command and getting reply
        ble_.atcommandStrReplyPerLine(F("AT+GATTLIST"), linebuf, 128, 100,
                                      fletcher16_comparator_callback, &comparator);
        if (!comparator.is_equal) {
            logger.print(F("GATTs incorrect\n"));
        }
        else {
            logger.print(F("GATTs correct\n"));
        }

        return comparator.is_equal;
    }

    void load_or_setup_gatts() {
        // NB: gatts_as_expected must be changed if GATT definition is changed
        if (!gatts_as_expected()) {
            // Reset the BLE module and recreate GATTs from scratch
            ble_.factoryReset();

            gatt_.clear();
            setup_cycling_power_feature();
            setup_cycling_speed_cadence_feature();

            // Store initialization to EEPROM
            EEPROM.update(EEPROM_BLE_CP_SERVICE_ID_ADDRESS,
                          (uint8_t) cp_service_id);
            EEPROM.update(EEPROM_BLE_CP_FEATURE_ID_ADDRESS,
                          (uint8_t) cp_feature_id);
            EEPROM.update(EEPROM_BLE_CP_MEASUREMENT_ID_ADDRESS,
                          (uint8_t) cp_measurement_id);
            EEPROM.update(EEPROM_BLE_CP_SENSOR_LOCATION_ID_ADDRESS,
                          (uint8_t) cp_sensor_location_id);
            EEPROM.update(EEPROM_BLE_CSC_SERVICE_ID_ADDRESS,
                          (uint8_t) csc_service_id);
            EEPROM.update(EEPROM_BLE_CSC_FEATURE_ID_ADDRESS,
                          (uint8_t) csc_feature_id);
            EEPROM.update(EEPROM_BLE_CSC_MEASUREMENT_ID_ADDRESS,
                          (uint8_t) csc_measurement_id);
            EEPROM.update(EEPROM_BLE_CSC_SENSOR_LOCATION_ID_ADDRESS,
                          (uint8_t) csc_sensor_location_id);
            EEPROM.update(EEPROM_BLE_SC_CONTROL_POINT_ID_ADDRESS,
                          (uint8_t) sc_control_point_id);
        } else {
            // Load IDs from EEPROM rather than reinitializing
            cp_service_id = EEPROM.read(EEPROM_BLE_CP_SERVICE_ID_ADDRESS);
            cp_feature_id = EEPROM.read(EEPROM_BLE_CP_FEATURE_ID_ADDRESS);
            cp_measurement_id = EEPROM.read(EEPROM_BLE_CP_MEASUREMENT_ID_ADDRESS);
            cp_sensor_location_id = EEPROM.read(EEPROM_BLE_CP_SENSOR_LOCATION_ID_ADDRESS);
            csc_service_id = EEPROM.read(EEPROM_BLE_CSC_SERVICE_ID_ADDRESS);
            csc_feature_id = EEPROM.read(EEPROM_BLE_CSC_FEATURE_ID_ADDRESS);
            csc_measurement_id = EEPROM.read(EEPROM_BLE_CSC_MEASUREMENT_ID_ADDRESS);
            csc_sensor_location_id = EEPROM.read(EEPROM_BLE_CSC_SENSOR_LOCATION_ID_ADDRESS);
            sc_control_point_id = EEPROM.read(EEPROM_BLE_SC_CONTROL_POINT_ID_ADDRESS);
        }
    }

    void setup_cycling_power_feature() {
        cp_service_id = gatt_.addService(CYCLING_POWER_SERVICE_UUID);

        // Add the three mandatory characteristics (table 3.1)
        // Cycling Power Feature
        cp_feature_id = gatt_.addCharacteristic(
            /* uuid          */ CYCLING_POWER_FEATURE_CHAR_UUID,
            /* properties    */ GATT_CHARS_PROPERTIES_READ,
            /* min_len       */ 4,
            /* max_len       */ 4,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);

        // Cycling Power Measurement
        cp_measurement_id = gatt_.addCharacteristic(
            /* uuid          */ CYCLING_POWER_MEASUREMENT_CHAR_UUID,
            /* properties    */ (GATT_CHARS_PROPERTIES_NOTIFY),
            /* min_len       */ 6,
            /* max_len       */ 6,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);

        // Sensor Location
        cp_sensor_location_id = gatt_.addCharacteristic(
            /* uuid          */ SENSOR_LOCATION_CHAR_UUID,
            /* properties    */ GATT_CHARS_PROPERTIES_READ,
            /* min_len       */ 1,
            /* max_len       */ 1,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);
    }

    void setup_cycling_speed_cadence_feature() {
        csc_service_id = gatt_.addService(CYCLING_SPEED_CADENCE_SERVICE_UUID);
        csc_feature_id = gatt_.addCharacteristic(
            /* uuid          */ CSC_FEATURE_CHAR_UUID,
            /* properties    */ GATT_CHARS_PROPERTIES_READ,
            /* min_len       */ 2,
            /* max_len       */ 2,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);

        // Cycling Speed/Cadence Measurement
        csc_measurement_id = gatt_.addCharacteristic(
            /* uuid          */ CSC_MEASUREMENT_CHAR_UUID,
            /* properties    */ (GATT_CHARS_PROPERTIES_NOTIFY),
            /* min_len       */ 11,
            /* max_len       */ 11,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);

        // Sensor Location
        csc_sensor_location_id = gatt_.addCharacteristic(
            /* uuid          */ SENSOR_LOCATION_CHAR_UUID,
            /* properties    */ GATT_CHARS_PROPERTIES_READ,
            /* min_len       */ 1,
            /* max_len       */ 1,
            /* datatype      */ BLE_DATATYPE_AUTO,
            /* description   */ NULL,
            /* presentFormat */ NULL);

        // SC Control Point
        sc_control_point_id = gatt_.addCharacteristic(
            /* uuid          */ SC_CONTROL_POINT_CHAR_UUID,
            /* properties    */ (GATT_CHARS_PROPERTIES_WRITE |
                                 GATT_CHARS_PROPERTIES_INDICATE),
            /* min_len       */ 1,
            /* max_len       */ SC_CONTROL_POINT_MAX_LENGTH,
            /* datatype      */ BLE_DATATYPE_AUTO, // ???
            /* description   */ NULL,
            /* presentFormat */ NULL);
    }

    bool update(const uint16_t crank_revs, const uint32_t last_crank_rev_timestamp_ms,
                const uint32_t wheel_revs, const uint32_t last_wheel_rev_timestamp_ms,
                uint16_t power_watts, const uint16_t total_energy_kj) {
        uint8_t data[11] = {0};
        uint8_t base;
        const bool update_cp = true;
        const bool update_csc = true;
        bool cpm_success = true;
        bool csc_success = true;
        if (update_cp) {
            // CP Measurement format specified in
            // https://github.com/oesmith/gatt-xml/blob/master/
            //    org.bluetooth.characteristic.cycling_power_measurement.xml

            /* NB: We will report wheel and crank revs in the CSC characteristic
             * rather than here. We'll only use CPM for power and energy.
             * CP and CSC use different time resolutions for wheel revs, and
             * exposing both according to their specs gives Wahoo a fit - never
             * figures out what the right speed is since they have different
             * time resolution.
             */

            base = 0;
            // flags: mandatory, 16 bit bitfield
            uint16_t flags = (CPM_ACCUMULATED_ENERGY_PRESENT);
            APPEND_BUFFER(data, base, flags);

            // Instantaneous power: mandatory sint16 in Watts
            // Clamp the uint16 input to avoid overflowing the sint16 expected by BT spec
            if (power_watts > 0x7FFF) power_watts = 0x7FFF;
            APPEND_BUFFER(data, base, power_watts);

            // 3.2.1.12 accumulated energy is in kJ uint16
            APPEND_BUFFER(data, base, total_energy_kj);

            cpm_success = gatt_.setChar(cp_measurement_id, data, base);
        }

        if (update_csc) {
            // Set up the CSC measurement with wheel and crank revs.
            // https://github.com/oesmith/gatt-xml/blob/master/
            // org.bluetooth.characteristic.csc_measurement.xml
            base = 0;
            // Flags: uint8
            uint8_t csc_flags = (CSCM_WHEEL_REV_DATA_PRESENT |
                                 CSCM_CRANK_REV_DATA_PRESENT);
            APPEND_BUFFER(data, base, csc_flags);

            // Cumulative wheel revs uint32
            APPEND_BUFFER(data, base, wheel_revs);
            // Last wheel rev event time: uint16, 1/1024s resolution
            // NB! Time resolution for wheel revs is lower in CSC than in CP!
            // CP would expect 1/2048.
            uint16_t last_wheel_event_time_csc = \
                (uint16_t) ((last_wheel_rev_timestamp_ms * 128) / 125);
            APPEND_BUFFER(data, base, last_wheel_event_time_csc);

            // Cumulative crank revs uint16
            APPEND_BUFFER(data, base, crank_revs);
            // Last Crank event time uint16 in 1/1024s units
            uint16_t last_crank_event_time = \
                (uint16_t) ((last_crank_rev_timestamp_ms * 128) / 125);
            APPEND_BUFFER(data, base, last_crank_event_time);

            csc_success = gatt_.setChar(csc_measurement_id, data, base);
        }

        handle_sc_control_point();
        return cpm_success && csc_success;
    }

    void handle_sc_control_point() {
        // We don't actually need to handle anything here for the Garmin to
        // connect. Some other devices might actually care about proper
        // responses.
        // In principle we could update the total number of wheel revs
        // but we don't persist that anyway.
        // This should probably be handled by ble_.setBleGattRxCallback()?
        return;
    }

    void serial_status_text() const {
        char buf[40];
        strcpy_P(buf, PSTR("\t\tBLECyclingPower:\n"));
        logger.print(buf);
        strcpy_P(buf, PSTR("\t\tCP SERVICE\n\t\tsid  fid  mid  slid\n"));
        logger.print(buf);
        snprintf_P(buf, 40, PSTR("\t\t% 3hhu  % 3hhu  % 3hhu  % 4hhu\n"),
                 cp_service_id, cp_feature_id, cp_measurement_id,
                 cp_sensor_location_id);
        logger.print(buf);
        strcpy_P(buf, PSTR("\t\tCSC SERVICE\n\t\tsid  fid  mid  slid\n"));
        logger.print(buf);
        snprintf_P(buf, 40, PSTR("\t\t% 3hhu  % 3hhu  % 3hhu  % 4hhu\n"),
                 csc_service_id, csc_feature_id, csc_measurement_id,
                 csc_sensor_location_id);
        logger.print(buf);
    }
};

#endif
