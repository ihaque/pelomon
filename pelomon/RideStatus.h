/* Peloton ride state tracker, handling power/energy integration,
 * crank speed integration, and speed computation.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
class RideStatus {
    private:
    Logger& logger;
    unsigned long last_rpm_timestamp;
    unsigned long last_power_timestamp;
    unsigned long last_crank_rev_timestamp;
    unsigned long last_wheel_rev_timestamp;
    float total_crank_revolutions;
    float total_wheel_revolutions;
    float total_energy_kj;
    float current_mph;
    uint16_t current_rpm;
    uint16_t current_power_deciwatt;
    uint16_t current_raw_resistance;
    uint8_t current_resistance;

    void update_revs_and_time(float& total_revs, unsigned long& event_ts,
                              const float incremental_revs, const float rev_per_ms,
                              const unsigned long ts) {
        const int previous_int_revs = (int) total_revs;
        total_revs += incremental_revs;
        const int integral_revs = (int) total_revs;
        if (previous_int_revs != integral_revs) {
            // Had at least one completed revolution event this time.
            // Need to compute how many ms back we last ticked over a rev
            float partial_revs = total_revs - integral_revs;
            float ms_since_last_cplt_rev = partial_revs / rev_per_ms;
            event_ts = ts - (unsigned long) ms_since_last_cplt_rev;
        }
        return;
    }
    float mph_from_power(const uint16_t power_deciwatts) const {
        // Derived from piecewise polynomial regression on a dataset of
        // about 150 rides. Regression done on watts but bike provides
        // watts * 10.
        const float power = ((float) power_deciwatts) * 0.1f;
        const float rtpower = sqrt(power);
        const float coefs_low[4] = {-0.07605, 0.74063, -0.14023, 0.04660};
        const float coefs_high[4] = {0.00087, -0.05685, 2.23594, -1.31158};
        const float* const coefs = power < 27.0f ? coefs_low : coefs_high;
        float mph = 0;
        for (uint8_t i=0; i < 3; i++) {
            mph += coefs[i];
            mph *= rtpower;
        }
        mph += coefs[3];
        return mph;
    }

    void update_new_rpm(const uint16_t new_rpm) {
        /* Update rpm and total crank revs since last rpm message.
         */
        const unsigned long ts = millis();
        if (last_rpm_timestamp == 0 || (ts - last_rpm_timestamp) > 5000) {
            // Reset our counter if we never saw data or saw it >5s ago
            last_rpm_timestamp = last_crank_rev_timestamp = ts;
            total_crank_revolutions = 0.0f;
        } 
        const unsigned long elapsed_ms = ts - last_rpm_timestamp;
        current_rpm = new_rpm;
        last_rpm_timestamp = ts;
        const float rpmsec_per_rpm = 1.0f / 60000.0f;
        const float rpmsec = rpmsec_per_rpm * current_rpm;
        const float increm_crank_revs = rpmsec * elapsed_ms;
        update_revs_and_time(total_crank_revolutions, last_crank_rev_timestamp,
                             increm_crank_revs, rpmsec, ts);
        if (total_crank_revolutions > 16777215.0f) {
             // Prevent FP32 loss of precision
             // It is ok for crank revolutions to roll over
             total_crank_revolutions -= (unsigned long) total_crank_revolutions;
        }
    }
    void update_new_power(const uint16_t new_power_deciwatts) {
        /* Update power, accumulated energy, current speed,
         * and total wheel revolutions since last power message.
         */
        const unsigned long ts = millis();
        if (last_power_timestamp == 0 || (ts - last_power_timestamp) > 5000) {
            // Reset our counter if we never saw data or saw it >5s ago
            last_power_timestamp = last_wheel_rev_timestamp = ts;
            total_energy_kj = total_wheel_revolutions = 0.0f;
        }
        // Update stored values
        const unsigned long elapsed_ms = ts - last_power_timestamp;
        last_power_timestamp = ts;
        current_power_deciwatt = new_power_deciwatts;
        current_mph = mph_from_power(current_power_deciwatt);

        // Integrate energy
        /* deciwatts/10 * seconds = joules
         * deciwatts/10 * ms/1000 = joules
         * deciwatts/10 * ms/1000 / 1000 = kilojoules
         * deciwatts * ms / 1e7 = kj
         */
        const float kj_per_deciwattms = 1e-7f;
        total_energy_kj += (current_power_deciwatt * elapsed_ms) * kj_per_deciwattms;

        // Integrate wheel revs
        /* Constant computed for 700c x 25 wheel at 2105mm
         *
         *  1 mi * 5280 ft * 12 in * 2.54 cm * 10 mm = 1603944 mm/mi
         *  1603944 mm / 2105mm = 764.53397 wheel revs per mi
         *  1 / 3.6e6 hr / msec
         *  1 mi/hr * 764.53397 rev/mi => 764.53397 rev/hr * 1/3.6e6 hr/msec
         *  => 2.1237e-4 rev/ms/mph
         */
        const float wheelrev_per_ms_mph = 2.1237e-4;
        const float rev_per_ms = wheelrev_per_ms_mph * current_mph;
        const float increm_wheel_revs = rev_per_ms * elapsed_ms;
        update_revs_and_time(total_wheel_revolutions, last_wheel_rev_timestamp,
                             increm_wheel_revs, rev_per_ms, ts);
        if (total_wheel_revolutions > 16777215.0f) {
             // Prevent FP32 loss of precision
             // Technically this is non-compliant - we should never roll over
             // But I don't want to deal with keeping the rest of the 32b elsewhere.
             total_wheel_revolutions -= (unsigned long) total_wheel_revolutions;
        }
    }
    void update_new_resistance(const uint16_t new_raw_resistance,
                               const ResistanceLUT& lut) {
        current_raw_resistance = new_raw_resistance;
        current_resistance = lut.translate_raw_resistance(current_raw_resistance);
    }
    public:
    RideStatus(Logger& logger_): logger(logger_) {};
    void initialize() {
        current_rpm = current_power_deciwatt = current_raw_resistance = current_resistance = 0;
        total_crank_revolutions = total_wheel_revolutions = total_energy_kj = current_mph = 0;
        last_rpm_timestamp = last_power_timestamp = 0;
        last_crank_rev_timestamp = last_wheel_rev_timestamp = 0;
    }
    uint16_t current_watts() const {
        uint16_t watts = current_power_deciwatt / 10;
        if (current_power_deciwatt % 10 >= 5) watts++;
        return watts;
    }
    uint16_t current_deciwatts() const {
        return current_power_deciwatt;
    }
    uint16_t total_kj() const {
        return (uint16_t) total_energy_kj;
    }
    uint32_t integral_wheel_revolutions() const {
        return (uint32_t) total_wheel_revolutions;
    }
    uint16_t integral_crank_revolutions() const {
        return (uint16_t) total_crank_revolutions;
    }
    uint32_t last_crank_rev_ts_millis() const {
        return last_crank_rev_timestamp;
    }
    uint32_t last_wheel_rev_ts_millis() const {
        return last_wheel_rev_timestamp;
    }
    void update(const BikeMessage& msg, const ResistanceLUT& lut) {
        char logbuf[32];
        if (!msg.is_valid) return false;
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            snprintf_P(logbuf, 32, PSTR("req: %hhu\n"), msg.request);
            logger.print(logbuf);
        }
        const unsigned long update_start = micros();
        if (msg.request == RPM) {
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) logger.print(F("Updating RPM\n"));
            update_new_rpm(msg.value);
       } else if (msg.request == POWER) {
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) logger.print(F("Updating power\n"));
            update_new_power(msg.value);
        } else if (msg.request == RESISTANCE) {
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) logger.print(F("Updating resistance\n"));
            update_new_resistance(msg.value, lut);
        } else {
            logger.print(F("DEFAULT CASE IN RIDESTATUS::UPDATE\n"));
            snprintf_P(logbuf, 32, "request %hhX\n",msg.request);
            logger.print(logbuf);
            while(1);
        }
        const unsigned long update_end = micros();
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            if (msg.request == RPM) {
                snprintf_P(logbuf, 32, PSTR("RPM upd %luus\n"), update_end-update_start);
            } else if (msg.request == POWER) {
                snprintf_P(logbuf, 32, PSTR("PWR upd %luus\n"), update_end-update_start);
            } else if (msg.request == RESISTANCE) {
                snprintf_P(logbuf, 32, PSTR("RES upd %luus\n"), update_end-update_start);
            }
            logger.print(logbuf);
        }
    }
    void serial_status_text() const {
        const int buflen = 128;
        char logbuf[128];
        char mph_str[5], crank_str[8], wheel_str[8], kj_str[9], power_str[8];
        snprintf_P(power_str, 8, PSTR("% 4u.%uW"),
                   current_power_deciwatt/10,
                   current_power_deciwatt % 10);
        dtostrf(current_mph, 4, 1, mph_str);
        dtostrf(total_energy_kj, 8, 3, kj_str);
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            snprintf_P(logbuf, buflen,
                               PSTR("\tRideStatus\n"
                                    "\t\trpm: %u @ lrt %lu\n"
                                    "\t\tpower: %s @ lpt %lu\n"
                               ),
                               current_rpm, last_rpm_timestamp,
                               power_str,
                               last_power_timestamp);
            logger.print(logbuf);
            dtostrf(total_crank_revolutions, 6, 2, crank_str);
            dtostrf(total_wheel_revolutions, 6, 2, wheel_str);
            snprintf_P(logbuf, buflen,
                       PSTR("\t\tspeed: %s mph\n"
                            "\t\tresistance: %hhu(%u)\n"),
                       mph_str,
                       current_resistance, current_raw_resistance);
            logger.print(logbuf);
            snprintf_P(logbuf, buflen,
                       PSTR("\t\tcranks: %s @ %lu\n"
                            "\t\twheels: %s @ %lu\n"
                            "\t\tenergy: %skJ\n"),
                        crank_str, last_crank_rev_timestamp,
                        wheel_str, last_wheel_rev_timestamp,
                        kj_str);
            logger.print(logbuf);
        } else if (LOG_LEVEL >= LOG_LEVEL_INFO) {
            snprintf_P(logbuf, buflen,
                       PSTR("% 3urpm %smph %s %skJ\n"),
                       current_rpm, mph_str, power_str, kj_str);
            logger.print(logbuf);
        }
    }
};
