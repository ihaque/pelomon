# Hardware Peloton emulator

Copyright 2020 Imran S Haque (ish@ihaque.org). Licensed under the
[CC-BY-NC 4.0 license](https://creativecommons.org/licenses/by-nc/4.0/).

# Description

Timing-accurate emulator for serial communications between the Peloton head unit
and bike. Emulates both HU and bike. Note that although communications are logically-
and timing-equivalent (e.g., inverted 19200bps serial communication), they are
*not* voltage-equivalent, as the normal logic output levels of the Arduino will be
used rather than the output levels from an RS-232 UART.

See more details in the
[accompanying blog post](https://ihaque.org/posts/2020/12/26/pelomon-part-ii-emulating-peloton) and series:
[https://ihaque.org/tag/pelomon.html](https://ihaque.org/tag/pelomon.html).

# Usage

Pin 10 outputs the data coming from the "bike" and pin 11 outputs the data from the
"head unit". Take care about logic/voltage levels. As written, the sketch outputs
inverted serial communications and is intended to be connected to an inverter/level
shifter upstream of the PeloMon MCU. This level shifter is *required* if using a
5V Arduino for the emulator. If you are using a 3.3V Arduino, you may directly
connect the two boards together and define `INVERTED_SERIAL` as `false`.

To use, upload to Arduino Uno or compatible. Keep A0 and A5 connected to GND during power on
until you see a slow blinking and PeloMon is initialized. Disconnect A5 to send bike
bootup sequence. When blink switches to pattern of 3 rapid blinks, bootup sequence is
complete. Disconnect A0 to start sending message pattern from a ride. Reconnect A0 to
GND to pause ride. To re-enter bootup sequence, you must reset the board (this is
fast!) -- make sure to ground A5 again.
