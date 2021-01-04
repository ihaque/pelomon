# The PeloMon

Source code for the PeloMon. See the accompanying blog post at
https://ihaque.org/posts/2021/01/04/pelomon-part-iv-software/.

# Building

This sketch is designed to be built and uploaded using the Arduino IDE onto
an Adafruit 32u4 Bluefruit LE. To set up your environment, follow the
[instructions from Adafruit](https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/setup)
on setting up the board environment; once that is complete, the sketch should
build normally.

When originally written, the sketch required a modified version of the
[Adafruit nRF51 support libraries](https://github.com/adafruit/Adafruit_BluefruitLE_nRF51)
and the modified files are therefore included in this repo. Since the original writing,
the modifications were [merged upstream](https://github.com/adafruit/Adafruit_BluefruitLE_nRF51/pull/56),
so the files may no longer be required here.

# License

All files copyright 2020 Imran S Haque (imran@ihaque.org) and licensed
under the [CC-BY-NC 4.0 license](https://creativecommons.org/licenses/by-nc/4.0/)
except for `Adafruit_*` and `utility/*` files, which are licensed under the
BSD license reflected in their source code.

