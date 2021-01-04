/* Debug message ring buffer.
 *
 * Part of the PeloMon project. See the accompanying blog post at
 * https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/
 *
 * Copyright 2020 Imran S Haque (imran@ihaque.org)
 * Licensed under the CC-BY-NC 4.0 license
 * (https://creativecommons.org/licenses/by-nc/4.0/).
 */
#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#ifdef ENABLE_RINGBUF
const uint8_t MSG_RINGBUF_LEN = 32;
uint8_t last_hu_msgs[MSG_RINGBUF_LEN];
uint32_t last_bike_msgs[MSG_RINGBUF_LEN];
unsigned long last_msg_times[MSG_RINGBUF_LEN];
uint8_t msg_index;

void dump_ringbuf(void) {
    char logbuf[32];
    for (uint8_t j=0; j < MSG_RINGBUF_LEN; j++) {
        uint8_t i = (msg_index + j) % MSG_RINGBUF_LEN;
        snprintf_P(logbuf, 32, PSTR("%lu: %hhx - %lx\n"),
                   last_msg_times[i], last_hu_msgs[i], last_bike_msgs[i]);
        logger.print(logbuf);
    }
}

void add_ringbuf(void) {
    last_msg_times[msg_index] = millis();
    last_hu_msgs[msg_index] = hu_buf[1];
    last_bike_msgs[msg_index] = bike_buf[3];
    last_bike_msgs[msg_index] <<= 8;
    last_bike_msgs[msg_index] |= bike_buf[4];
    last_bike_msgs[msg_index] <<= 8;
    last_bike_msgs[msg_index] |= bike_buf[5];
    last_bike_msgs[msg_index] <<= 8;
    last_bike_msgs[msg_index] |= bike_buf[6];
    msg_index = (msg_index + 1) % MSG_RINGBUF_LEN;
}

void init_ringbuf(void) {
    memset(last_hu_msgs, 0, MSG_RINGBUF_LEN);
    memset(last_bike_msgs, 0, 4*MSG_RINGBUF_LEN);
    memset(last_msg_times, 0, 4*MSG_RINGBUF_LEN);
    msg_index = 0;
}

#else
#define add_ringbuf()
#define init_ringbuf()
#define dump_ringbuf()
#endif

#endif
