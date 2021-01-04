# Peloton interface library and software emulator

Copyright 2020 Imran S Haque (ish@ihaque.org). Licensed under the
[CC-BY-NC 4.0 license](https://creativecommons.org/licenses/by-nc/4.0/).

# Description

Event-driven emulator for Peloton Bike behavior, easily directly integrated into
a sketch for Peloton interfacing. Also includes routines to parse and validate
messages from the bike and HU.

See more details in the
[accompanying blog post](https://ihaque.org/posts/2020/12/26/pelomon-part-ii-emulating-peloton) and series:
[https://ihaque.org/tag/pelomon.html](https://ihaque.org/tag/pelomon.html).

# Emulator usage

Add the header to your sketch: `#include "peloton.h"`. Settings are defined inline but can be
defined in an earlier header instead; check the `#ifdef` at the top of `peloton.h` to see details.

The emulator is implemented in class `PelotonSimulator`. To use directly, instantiate a `PelotonSimulator`
object, and treat the `hu` and `bike` attributes as you would a `SoftwareSerial` object.

If dealing with both real hardware and emulated hardware in the same sketch, it is easier to instead
use the `PelotonProxy` class.  Instantiate a `PelotonProxy` object and initialize it with a boolean
indicating whether to use emulation (`true`) or the underlying SoftwareSerial interfaces connected
to real hardware (or a hardware emulator) (`false`).

```
PelotonProxy peloton;
// ...
void setup() {
    // ...
    peloton.initialize(use_emulation);  
    // ...
}
```

Then, instead of interfacing with SoftwareSerial objects/interfaces directly, use the proxy object, which
handles switching between emulated and real interfaces. Given interfaces `SoftwareSerial hu, bike` vs
`PelotonProxy peloton`:

- `hu.available()` -> `proxy.hu_available()`
- `bike.available()` -> `proxy.bike_available()`
- `hu.listen()` -> `proxy.hu_listen()`
- `bike.listen()` -> `proxy.bike_listen()`
- `hu.read()` -> `proxy.hu_read()`
- `bike.read()` -> `proxy.bike_read()`

# Parser usage

`peloton.h` implements two classes `BikeMessage` and `HUMessage` parsing packets from the Peloton Bike and
its head unit, respectively, as well as two `enum`s `HUPacketType` and `Requests` enumerating valid requests
from Peloton communications.

## `HUPacketType` and `Requests`
Two enumerations providing human-readable names for Peloton communications, following the
[protocol as previously decoded](https://ihaque.org/posts/2020/10/15/pelomon-part-i-decoding-peloton/):

<table>
<tr><th>Name</th><th>Value</th><th>Description</th></tr>
<tr><td colspan="3"><tt>HUPacketType</tt></td></tr>
<tr><td><tt>STARTUP_UNKNOWN</tt></td><td>0xFE</td><td>Unknown packet sent once, first, at boot</td></tr>
<tr><td><tt>READ_FROM_BIKE</tt></td><td>0xF5</td><td>Read value from bike</td></tr>
<tr><td><tt>READ_RESISTANCE_TABLE</tt></td><td>0xF5</td><td>Read value from bike's resistance calibration table</td></tr>
<tr><td colspan="3"><tt>Requests</tt></td></tr>
<tr><td><tt>RPM</tt></td><td>0x41</td><td>Request (in HU message) or return (in bike message) current cadence</td></tr>
<tr><td><tt>POWER</tt></td><td>0x44</td><td>Request (in HU message) or return (in bike message) current power</td></tr>
<tr><td><tt>RESISTANCE</tt></td><td>0x44</td><td>Request (in HU message) or return (in bike message) current raw resistance</td></tr>
<tr><td><tt>BIKE_ID</tt></td><td>0xFB</td><td>Request (in HU message) or return (in bike message) bike ID</td></tr>
<tr><td><tt>UNKNOWN_INIT_REQUEST</tt></td><td>0xFE</td><td>Return (in bike message) response to unknown init request at boot</td></tr>
<tr><td><tt>READ_RESISTANCE_TABLE_(00...1E)</tt></td><td>0x00 - 0x1E</td><td>Request (in HU message) resistance calibration table value from 0x00 through 0x1E</td></tr>
<tr><td><tt>RESISTANCE_TABLE_RESPONSE</tt></td><td>0xF7</td><td>Return (in bike message) resistance calibration table last requested by head unit (NB: which address is unspecified in the return!)</td></tr>
</table>

## `HUMessage`
`HUMessage::HUMessage(uint8_t* msg, const uint8_t len)` parses a raw byte buffer into a head unit request message,
with the following fields:

- `bool HUMessage.is_valid`: `true` if the message had an appropriate length, header byte, request, and checksum.
- `HUPacketType HUMessage.packet_type`: indicates type of packet
- `Requests HUMessage.request`: indicates particular request HU made

## `BikeMessage`
`BikeMessage::BikeMessage(uint8_t* msg, const uint8_t len)` parses a raw byte buffer into a bike response message,
with the following fields:

- `bool BikeMessage.is_valid`: `true` if the message had an appropriate length, header byte, request, and checksum.
- `Requests BikeMessage.request`: indicates response type in this packet
- `uint16_t value`: For response types with a numerical value (resistance table readback, cadence, power, current
  raw resistance), the decoded numeric value from the packet. Otherwise zero.
