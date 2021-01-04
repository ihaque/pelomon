/* Human-readable constants for the Bluetooth LE Cycling Power Profile
 * and Cycling Speed and Cadence Profile.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef BLE_CONSTANTS_H
#define BLE_CONSTANTS_H

/* Constants for parameters related to BLE Cycling Power Service
 * and Cycling Speed and Cadence Service and their related
 * GATT Characteristics.
 */

#define CYCLING_POWER_SERVICE_UUID ((uint16_t) 0x1818)

#define CYCLING_SPEED_CADENCE_SERVICE_UUID ((uint16_t) 0x1816)

#define SC_CONTROL_POINT_CHAR_UUID ((uint16_t) 0x2A55)
#define SC_CONTROL_POINT_MAX_LENGTH 5
#define SC_CONTROL_POINT_OP_SET_CUMULATIVE_VALUE ((uint8_t) 1)
#define SC_CONTROL_POINT_OP_START_SENSOR_CALIBRATION ((uint8_t) 2)
#define SC_CONTROL_POINT_OP_UPDATE_SENSOR_LOCATION ((uint8_t) 3)
#define SC_CONTROL_POINT_OP_REQUEST_SUPPORTED_SENSOR_LOCATIONS ((uint8_t) 4)
#define SC_CONTROL_POINT_RESPONSE_SUCCESS ((uint8_t) 1)
#define SC_CONTROL_POINT_RESPONSE_OPCODE_NOT_SUPPORTED ((uint8_t) 2)
#define SC_CONTROL_POINT_RESPONSE_INVALID_PARAMETER ((uint8_t) 3)
#define SC_CONTROL_POINT_RESPONSE_OPERATION_FAILED ((uint8_t) 4)

#define CSC_MEASUREMENT_CHAR_UUID ((uint16_t) 0x2A5B)
#define CSCM_WHEEL_REV_DATA_PRESENT (1 << 0)
#define CSCM_CRANK_REV_DATA_PRESENT (1 << 1)

#define CSC_FEATURE_CHAR_UUID ((uint16_t) 0x2A5C)
#define CSCF_WHEEL_REVOLUTION_DATA_SUPPORTED ((uint16_t) 1 << 0)
#define CSCF_CRANK_REVOLUTION_DATA_SUPPORTED ((uint16_t) 1 << 1)
#define CSCF_MULTIPLE_SENSOR_LOCATIONS_SUPPORTED ((uint16_t) 1 << 2)

#define SENSOR_LOCATION_CHAR_UUID ((uint16_t) 0x2A5D)
#define SENSOR_LOCATION_OTHER ((uint8_t) 0)
#define SENSOR_LOCATION_TOP_OF_SHOE ((uint8_t) 1)
#define SENSOR_LOCATION_IN_SHOE ((uint8_t) 2)
#define SENSOR_LOCATION_HIP ((uint8_t) 3)
#define SENSOR_LOCATION_FRONT_WHEEL ((uint8_t) 4)
#define SENSOR_LOCATION_LEFT_CRANK ((uint8_t) 5)
#define SENSOR_LOCATION_RIGHT_CRANK ((uint8_t) 6)
#define SENSOR_LOCATION_LEFT_PEDAL ((uint8_t) 7)
#define SENSOR_LOCATION_RIGHT_PEDAL ((uint8_t) 8)
#define SENSOR_LOCATION_FRONT_HUB ((uint8_t) 9)
#define SENSOR_LOCATION_REAR_DROPOUT ((uint8_t) 10)
#define SENSOR_LOCATION_CHAINSTAY ((uint8_t) 11)
#define SENSOR_LOCATION_REAR_WHEEL ((uint8_t) 12)
#define SENSOR_LOCATION_REAR_HUB ((uint8_t) 13)
#define SENSOR_LOCATION_CHEST ((uint8_t) 14)
#define SENSOR_LOCATION_SPIDER ((uint8_t) 15)
#define SENSOR_LOCATION_CHAIN_RING ((uint8_t) 16)

#define CYCLING_POWER_FEATURE_CHAR_UUID ((uint16_t) 0x2A65)
#define CPF_PEDAL_POWER_BALANCE_SUPPORTED ((uint32_t) 1 << 0)
#define CPF_ACCUMULATED_TORQUE_SUPPORTED ((uint32_t) 1 << 1)
#define CPF_WHEEL_REVOLUTION_DATA_SUPPORTED ((uint32_t) 1 << 2)
#define CPF_CRANK_REVOLUTION_DATA_SUPPORTED ((uint32_t) 1 << 3)
#define CPF_EXTREME_MAGNITUDES_SUPPORTED ((uint32_t) 1 << 4)
#define CPF_EXTREME_ANGLES_SUPPORTED ((uint32_t) 1 << 5)
#define CPF_TOP_BOTTOM_DEAD_SPOT_ANGLES_SUPPORTED ((uint32_t) 1 << 6)
#define CPF_ACCUMULATED_ENERGY_SUPPORTED ((uint32_t) 1 << 7)
#define CPF_OFFSET_COMPENSATION_INDICATOR_SUPPORTED ((uint32_t) 1 << 8)
#define CPF_OFFSET_COMPENSATION_SUPPORTED ((uint32_t) 1 << 9)
#define CPF_CPM_CHAR_CONTENT_MASKING_SUPPORTED ((uint32_t) 1 << 10)
#define CPF_MULTIPLE_SENSOR_LOCATIONS_SUPPORTED ((uint32_t) 1 << 11)
#define CPF_CRANK_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t) 1 << 12)
#define CPF_CHAIN_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t) 1 << 13)
#define CPF_CHAIN_WEIGHT_ADJUSTMENT_SUPPORTED ((uint32_t) 1 << 14)
#define CPF_SPAN_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t) 1 << 15)
#define CPF_SENSOR_MEASUREMENT_CONTEXT_TORQUE_BASED ((uint32_t) 1 << 16)
#define CPF_INSTANTANEOUS_MEASUREMENT_DIRECTION_SUPPORTED ((uint32_t) 1 << 17)
#define CPF_FACTORY_CALIBRATION_DATE_SUPPORTED ((uint32_t) 1 << 18)
#define CPF_ENHANCED_OFFSET_COMPENSATION_SUPPORTED ((uint32_t) 1 << 19)


#define CYCLING_POWER_MEASUREMENT_CHAR_UUID ((uint16_t) 0x2A63)
#define CPM_FLAG_PEDAL_POWER_BALANCE_PRESENT ((uint16_t) 1 << 0)
#define CPM_FLAG_PEDAL_POWER_BALANCE_REFERENCE_LEFT ((uint16_t) 1 << 1)
#define CPM_ACCUMULATED_TORQUE_PRESENT ((uint16_t) 1 << 2)
#define CPM_ACCUMULATED_TORQUE_CRANK_BASED ((uint16_t) 1 << 3)
#define CPM_WHEEL_REV_DATA_PRESENT ((uint16_t) 1 << 4)
#define CPM_CRANK_REV_DATA_PRESENT ((uint16_t) 1 << 5)
#define CPM_EXTREME_FORCE_MAGNITUDES_PRESENT ((uint16_t) 1 << 6)
#define CPM_EXTREME_TORQUE_MAGNITUDES_PRESENT ((uint16_t) 1 << 7)
#define CPM_EXTREME_ANGLES_PRESENT ((uint16_t) 1 << 8)
#define CPM_TOP_DEAD_SPOT_ANGLE_PRESENT ((uint16_t) 1 << 9)
#define CPM_BOTTOM_DEAD_SPOT_ANGLE_PRESENT ((uint16_t) 1 << 10)
#define CPM_ACCUMULATED_ENERGY_PRESENT ((uint16_t) 1 << 11)
#define CPM_OFFSET_COMPENSATION_ACTION_REQUIRED ((uint16_t) 1 << 12)

#endif
