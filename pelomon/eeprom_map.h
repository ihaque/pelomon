/* EEPROM memory map for the PeloMon
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef EEPROM_MAP_H
#define EEPROM_MAP_H
/* EEPROM map
 *  0: resistance value 0, low byte
 *  1: resistance value 0, high byte
 *  2: resistance value 1, low byte
 *  ...
 *  60: resistance value 1E, low byte
 *  61: resistance value 1E, high byte
 *  62: resistance checksum, low byte
 *  63: resistance checksum, high byte
 *  64: Force Bike Simulator at Startup
 *  65: BLE: Cycling Power Service ID
 *  66: BLE: Cycling Power Feature GATT ID
 *  67: BLE: Cycling Power Measurement GATT ID
 *  68: BLE: Cycling Power Sensor Location GATT ID
 *  69: BLE: Cycling Speed/Cadence Service ID
 *  70: BLE: Cycling Speed/Cadence Feature GATT ID
 *  71: BLE: Cycling Speed/Cadence Measurement GATT ID
 *  72: BLE: Cycling Speed/Cadence Sensor Location GATT ID
 *  73: BLE: Cycling Speed/Cadence Control Point GATT ID
 */
enum _eeprom_map {
        EEPROM_RESISTANCE_LUT_BASE_ADDRESS = 0,
        EEPROM_RESISTANCE_LUT_CHECKSUM_ADDRESS = 62,
        EEPROM_FORCE_SIMULATION_AT_STARTUP = 64,
        EEPROM_BLE_CP_SERVICE_ID_ADDRESS,
        EEPROM_BLE_CP_FEATURE_ID_ADDRESS,
        EEPROM_BLE_CP_MEASUREMENT_ID_ADDRESS,
        EEPROM_BLE_CP_SENSOR_LOCATION_ID_ADDRESS,
        EEPROM_BLE_CSC_SERVICE_ID_ADDRESS,
        EEPROM_BLE_CSC_FEATURE_ID_ADDRESS,
        EEPROM_BLE_CSC_MEASUREMENT_ID_ADDRESS,
        EEPROM_BLE_CSC_SENSOR_LOCATION_ID_ADDRESS,
        EEPROM_BLE_SC_CONTROL_POINT_ID_ADDRESS,
        EEPROM_MAX_ADDRESS
};
#endif
