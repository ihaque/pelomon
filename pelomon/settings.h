/* Configuration for the PeloMon.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#define SIMULATOR_MESSAGE_INTERVAL_MILLIS 100

#define BT_UPDATE_INTERVAL_MILLIS 500

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_MAX   255

/*
 *  Pin usage and available pins
 *  https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/pinouts
 *  https://learn.adafruit.com/assets/46242
 *  IN USE by onboard hardware:
 *      0, 1 (Hardware serial)
 *      SCK, MOSI, MISO (SPI/BLE module)
 *      9/A9 (has PCINT) -- connected to resistor divider on BAT
 *      13 -- connected to red LED
 *  RESERVED as a consequence of board layout
 *      NONE
 *  IN USE in current design
 *       6 -- default high, pull low to force simulation (no PCINT)
 *      10 -- RX from bike (PCINT)
 *      11 -- RX from HU (PCINT)
 *  Available for use:
 *      A0-A5, 5, 12 (no PCINT)
 *      2/SDA, 3/SCL (no PCINT) [Reserved for TX]
 */

#define PIN_LOW_FORCE_SIM         6     // tie to GND to force simulator
#define PIN_RX_FROM_BIKE          10
#define PIN_RX_FROM_HU            11
#define PIN_TX_TO_HU              3     // currently unused
#define PIN_TX_TO_BIKE            2     // currently unused
#define PIN_STATE_READ_HU         A0
#define PIN_STATE_READ_BIKE       A1
#define PIN_STATE_PROC_MSG        A2
#define PIN_STATE_LISTEN_HU       A3
#define PIN_STATE_LISTEN_BIKE     A4
#define PIN_STATE_HANDLE_CMD      A5
// If there is a hardware inverter in the RX chain we do not
// need to invert the SoftwareSerial sense. If no inverter, then
// we gotta do it in software.
#define INVERT_PELOTON_SERIAL false

// Bluefruit settings
// SHARED SPI SETTINGS
// ----------------------------------------------------------------------------
// The following macros declare the pins to use for HW and SW SPI communication.
// SCK, MISO and MOSI should be connected to the HW SPI pins on the Uno when
// using HW SPI.  This should be used with nRF51822 based Bluefruit LE modules
// that use SPI (Bluefruit LE SPI Friend).
// ----------------------------------------------------------------------------
#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4    // Optional but recommended, set to -1 if unused


