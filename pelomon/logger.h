/* Logging utilities.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef _LOGGER_H_
#define _LOGGER_H_
#include <avr/pgmspace.h>

#ifndef MIN
#define MIN(x,y) (x) < (y) ? (x) : (y)
#endif


class Logger {
    private:
        Adafruit_BLE* ble_;
    public:
        Logger(): ble_(NULL) {}
        void set_ble(Adafruit_BLE* ble) {
            ble_ = ble;
        }
        size_t write(uint8_t const* buf, const size_t len) {
            size_t written = 0, towrite = 0, ble_written = 0;
            // If nonblocking, conditionally truncate writes
            const bool nonblock = false;
            if (Serial) {
                towrite = nonblock ? MIN(Serial.availableForWrite(), len) : len;
                written = Serial.write(buf, towrite);
            }
            if (ble_ != NULL) {
                ble_written = ble_->writeBLEUart(buf, len);
            }
            return ble_written > written ? ble_written : written;
        }
        size_t print(char c) {
            return write(&c, 1);
        }
        size_t print(char const* str) {
            size_t len = strlen(str);
            return write(str, len);
        }
        size_t println(char const* str) {
            const char newline = '\n';
            return print(str) + write(&newline, 1);
        }
        size_t print(const __FlashStringHelper* Fstr) {
            uint8_t buf[63];
            char const* Pstr = (char const*)Fstr;
            size_t len = strlen_P(Pstr);
            size_t written = 0;
            for (size_t base=0; base < len; ) {
                size_t nbytes = len - base;
                nbytes = nbytes > 63 ? 63 : nbytes;
                memcpy_P(buf, Pstr + base, nbytes);
                written += write(buf, nbytes);
                base += nbytes;
            }
            return written;
        }
        size_t println(const __FlashStringHelper* Fstr) {
            const char newline = '\n';
            return print(Fstr) + write(&newline, 1);
        }
};
#endif
