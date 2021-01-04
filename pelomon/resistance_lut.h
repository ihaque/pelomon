/* Peloton resistance lookup table mapping raw resistance to 0-100 resistance.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#include "eeprom_map.h"

#ifndef RESISTANCE_LUT_H
#define RESISTANCE_LUT_H
#define TABLE_SPACING (100.0f / 30.0f)
class ResistanceLUT
{
  private:
    Logger& logger;
    uint16_t lut[31];
    bool valid_;
    bool synced;
    
    uint16_t compute_checksum() {
        uint16_t checksum = 0;
        for (uint8_t i = 0; i < 31; checksum += lut[i++]);
        checksum ^= 0xB01D;
        return checksum;
    }

    uint16_t read_uint16t(int address) {
        return (EEPROM.read(address + 1) << 8) | EEPROM.read(address);
    }

    void update_uint16t(int address, uint16_t value) {
        EEPROM.update(address, value & 0xFF);
        EEPROM.update(address + 1, (value >> 8) & 0xFF);
    }
  
  public:
      ResistanceLUT(Logger& logger_): logger(logger_) {};
      void initialize() {
          // Load LUT from EEPROM
          uint8_t address = EEPROM_RESISTANCE_LUT_BASE_ADDRESS;
          for (uint8_t i = 0; i < 31; i++) {
              lut[i] = read_uint16t(address);
              address += 2;
          }
          uint16_t stored_checksum = read_uint16t(EEPROM_RESISTANCE_LUT_CHECKSUM_ADDRESS);
          uint16_t checksum = compute_checksum();
          valid_ = (checksum == stored_checksum);
          if (!valid_) {
              // Reset the LUT with sentinels if it was not valid
              // This way we know if all entries were initialized before saving.
              for (uint8_t i = 0; i < 31; lut[i++] = 0xFFFF);
          }
          synced = true;
      }
      bool is_valid() {
          if (!synced) {
              valid_ = true;
              for (uint8_t i = 0; i < 31; i++) {
                  if (lut[i] == 0xFFFF) valid_ = false;
                  // Ensure monotonicity
                  if (i > 0 && lut[i] <= lut[i-1]) valid_ = false;
              }
          }
          return valid_;
      }
      bool update_entry(uint16_t raw_value, uint8_t index) {
        if (index > 30) {
            return false;
        }
        lut[index] = raw_value;
        synced = false;
        valid_ = valid_ && raw_value != 0xFFFF;
      }
      void sync_to_eeprom() {
          if (!is_valid()) return;
          uint8_t address = EEPROM_RESISTANCE_LUT_BASE_ADDRESS;
          for (uint8_t i = 0; i < 31; i++) {
              update_uint16t(address, lut[i]);
              address += 2;
          }
          update_uint16t(EEPROM_RESISTANCE_LUT_CHECKSUM_ADDRESS,
                         compute_checksum());
          synced = true;
      }
      uint8_t translate_raw_resistance(const uint16_t raw_resistance) {
          // Invalid LUT
          if (!is_valid()) return 0xFF;
          // Out of range 
          if (raw_resistance < lut[0] || raw_resistance > lut[30]) return 0xFF;

          // Linear search to find bounding interval
          uint8_t lb;
          for (lb = 0; lb < 29; lb++) {
              if (raw_resistance >= lut[lb] && raw_resistance <= lut[lb+1])
                  break;
          }
          // The 31 samples correspond to values @ 0, 3.3, 6.7, 10, ...
          float proportion = ((raw_resistance - lut[lb]) /
                              ((float) (lut[lb + 1] - lut[lb])));
          return (uint8_t) (TABLE_SPACING * (proportion + lb));
      }
      void serial_status_text() const {
          char buf[48];
          snprintf_P(buf, 48,
                     PSTR("\tResistanceLUT\n"
                          "\t\tvalid: %d synced: %d\n"
                          "\t\tLUT:\n"),
                     (int)valid_, (int)synced);
          logger.print(buf);
          for (uint8_t base=0; base<28; base += 4) {
              snprintf_P(buf, 32, PSTR("\t\t% 5d % 5d % 5d % 5d\n"),
                       lut[base+0], lut[base+1], lut[base+2], lut[base+3]);
              logger.print(buf);
          }
          snprintf_P(buf, 32, PSTR("\t\t% 5d % 5d % 5d\n"),
                   lut[28], lut[29], lut[30]);
          logger.print(buf);
      }
};
#endif
