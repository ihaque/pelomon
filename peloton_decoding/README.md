# Scripts decoding Peloton ride communications

See the accompanying blog post at https://ihaque.org/posts/2020/10/15/pelomon-part-i-decoding-peloton/.

Code:
- `decoder_plots.py` contains some simple plots to visualize the dump of an example ride to understand
the structure of the Peloton data.
- `decode_resistance.py` contains plots to illustrate how resistance is normalized for the Peloton.
- `peloton.py` contains a module to parse a bytestream of binary communications between the Peloton
HU and Bike into more useful structures.

Data:

- `example-ride.txt` contains a dump of messages observed from the bike during the course of an
example ride, in text hex form, aligned by message.
- `resistance-stepped-10s.bin` contains a binary dump of all messages between the head unit and bike
during a test ride in which I stepped the resistance in discrete chunks.
- `resistance-stepped-10a.sr` - Sigrok/PulseView session file for the above ride. Not used in the scripts
but provided in case you'd like to load a trace into PulseView to see what it looks like.
