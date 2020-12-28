# Schematics for the PeloMon

Copyright 2020 Imran S Haque (ish@ihaque.org). Licensed under the
[CC-NY-NC 4.0 license](https://creativecommons.org/licenses/by-nc/4.0/).

For more details and a component list, see the accompanying blog post at
[https://ihaque.org/posts/2020/12/28/pelomon-part-iii-hardware](https://ihaque.org/posts/2020/12/28/pelomon-part-iii-hardware).

`breadboard.fzz` and `mint-tin-permaproto.fzz` are [Fritzing](https://fritzing.org) schematics for
the PeloMon. The former is laid out for a standard half-size breadboard. The latter is laid out for
the Adafruit [Mint tin-sized PermaProto](https://www.adafruit.com/product/723); while it is
illustrated using a half-sized breadboard (for performance reasons -- the Adafruit Fritzing library
version of the PermaProto has terrible Fritzing UI performance), the coordinates are correct
for a PermaProto layout.

Note that in both layouts, four pins for the 74AHCT14N -- pins 2,3,4,5 on the left side -- should
be clipped or otherwise left disconnected, as they overlap pins on the MPM3610 converter.
