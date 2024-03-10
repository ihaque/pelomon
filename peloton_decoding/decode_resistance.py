#!/usr/bin/env python

# Copyright 2020 Imran S. Haque https://ihaque.org
# Licensed under the GNU General Public License (GPL) v3

from itertools import groupby
import sys
import numpy as np
import matplotlib.pyplot as plt

class PelotonMessage(object):
    def __init__(self, data):
        if data[0] in (0xF5, 0xFE, 0xF7):
            self.source = 'HU'
        elif data[0] == 0xF1:
            self.source = 'bike'
        else:
            raise ValueError('Unknown source %02x' % data[0])
        if self.source == 'HU':
            self.request = data[0] << 8 | data[1]
            self.length = 4
            self.data = None
        else:
            # start, request, leength; cheksum, end; data
            self.request = data[1]
            self.length = 3 + 2 + data[2]
            self.data = data[3:self.length-2]
        checksum = sum(x for x in data[:self.length-2]) & 0xFF
        data_checksum = data[self.length-2]
        if checksum != data_checksum:
            raise ValueError('Invalid checksum')
        if data[self.length-1] != 0xF6:
            raise ValueError('Expected packet to end in F6, ended with %02x' % data[self.length-1])
        if self.source == 'bike':
            stringdata = self.data.decode('ascii')[::-1]
            intdata = int(stringdata, 10)
            #print(self.request, stringdata)
            if self.request == 0x41:
                self.type_ = 'rpm'
                self.rpm = intdata
            elif self.request == 0x44:
                self.type_ = 'power'
                self.watts = intdata / 10
            elif self.request == 0x4a:
                self.type_ = 'resistance'
                self.resistance = intdata
        else:
            self.type = None
    def __str__(self):
        fields = ['source: %s' % self.source,
                  'request: %02x' % self.request,
                  'data: %s' % 'None' if self.data is None else ' '.join('%02x' % i for i in self.data)]
        if self.source == 'bike':
            fields.append('type: %s' % self.type_)
            if self.type_ == 'rpm':
                fields.append('rpm: %d W' % self.rpm)
            elif self.type_ == 'power':
                fields.append('power: %f W' % self.watts)
            elif self.type_ == 'resistance':
                fields.append('resistance: %d' % self.resistance)
        return '; '.join(fields)
                                                        
                                                        

    @staticmethod
    def parse_stream(stream):
        offset = 0
        offset_limit = 10
        # Maximum packet size is 10 bytes
        while offset < offset_limit:
            try:
                message = PelotonMessage(stream[offset:])
                break
            except ValueError:
                offset += 1
        if offset == offset_limit:
            raise ValueError('Unable to synchronize to stream')
        stream = stream[offset:]
        while True:
            try:
                packet = PelotonMessage(stream)
            except IndexError:
                break
            stream = stream[packet.length:]
            yield packet
        return

if __name__ == '__main__':
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
