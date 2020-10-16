#!/usr/bin/env python

# Copyright 2020 Imran S. Haque https://ihaque.org
# Licensed under the GNU General Public License (GPL) v3

from itertools import groupby
import sys
import numpy as np
import matplotlib.pyplot as plt
from peloton import PelotonMessage

with open('resistance-stepped-10s.bin', 'rb') as df:
    bytestream = df.read()

# The resistance tiers used in the test run. Each was held in sequence for
# ten seconds.
resistance_settings = [ 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
                       95, 85, 75, 65, 55, 45, 35, 25, 15,  5,   0]

bike_bootup_replies = [
    164, 169, 186, 222, 297, 369, 440, 497,
    558, 609, 653, 687, 726, 757, 783, 803,
    827, 845, 861, 874, 889, 901, 911, 921,
    930, 938, 944, 952, 958, 963, 967]

message_stream = list(PelotonMessage.parse_stream(bytestream))
resistance = np.array([msg.resistance for msg in message_stream
                       if msg.source == 'bike' and
                       msg.type_ == 'resistance'])

resistance_diff = np.hstack([[0], np.diff(resistance)])
# Window of allowance for any "stable" setting - no more than +/- 1 variance
stable = np.abs(resistance_diff) < 2
chunks = []
for is_stable, group in groupby(enumerate(stable), key=lambda x: x[1]):
        if not is_stable:
            continue
        group = list(group)
        # Kept each setting for 10s and the bike gets polled for resistance
        # every 3 x 0.1 = 0.3s, so any chunks shorter than this
        # are not real.
        if len(group) < 30:
            continue
        indices = range(group[0][0], group[-1][0])
        chunk = resistance[group[0][0]:group[-1][0]]
        chunks.append((indices, chunk))

assert(len(chunks) == len(resistance_settings))
fig = plt.figure()
for (xs, ys), label in zip(chunks, resistance_settings):
    plt.scatter(xs, ys, label=str(label))
plt.legend(ncol=3)
plt.xlabel('Sample', size='x-large')
plt.ylabel('Raw Resistance', size='x-large')

plt.figure()
mean_replies = np.array([chunk.mean() for indices, chunk in chunks])
plt.scatter(resistance_settings, mean_replies)
plt.xlabel('Indicated resistance', size='x-large')
plt.ylabel('Raw resistance', size='x-large')

plt.figure()
plt.scatter(resistance_settings, mean_replies, label='Measured')
plt.scatter(np.linspace(0,100,len(bike_bootup_replies)),
            bike_bootup_replies, color='r',
            label='Bike bootup messages')
plt.legend()
plt.xlabel('Indicated resistance', size='x-large')
plt.ylabel('Raw resistance', size='x-large')

plt.show()
