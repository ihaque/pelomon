#!/usr/bin/env python

# Copyright 2020 Imran S. Haque https://ihaque.org
# Licensed under the GNU General Public License (GPL) v3

import matplotlib.pyplot as plt
import numpy as np

def bytewise_plot(responses_41, responses_44, responses_4a):
    fig = plt.figure()
    base = 1
    for i in range(responses_41.shape[1]):
        plt.subplot(3,5,base + i)
        plt.plot(responses_41[:,i])
        plt.title('41 byte %d' % (i+1))
    base = 6
    for i in range(responses_44.shape[1]):
        plt.subplot(3,5,base + i)
        plt.plot(responses_44[:,i])
        plt.title('44 byte %d' % (i+1))
    base = 11
    for i in range(responses_4a.shape[1]):
        plt.subplot(3,5,base + i)
        plt.plot(responses_4a[:,i])
        plt.title('4a byte %d' % (i+1))
    return fig


def ascii_decode(responses):
    values = [''.join(map(chr, responses[i,::-1]))
              for i in range(responses.shape[0])]
    return np.array([int(x) for x in values])


def ascii_plot(responses_41, responses_44, responses_4a):
    fig = plt.figure()
    plt.subplot(3,1,1)
    plt.plot(ascii_decode(responses_41))
    plt.title('Request type 0x41')
    plt.subplot(3,1,2)
    plt.plot(ascii_decode(responses_44))
    plt.title('Request type 0x44')
    plt.subplot(3,1,3)
    plt.plot(ascii_decode(responses_4a))
    plt.title('Request type 0x4a')

    return fig


if __name__ == '__main__':
    rows = []
    # Load and parse text dump of responses for test ride
    with open('example-ride.txt', 'r') as df:
        for line in df:
            rows.append([int(x, 16) for x in line.rstrip().split()])
    rows = np.array(rows)
    responses_41 = rows[:,0:3]
    responses_44 = rows[:,3:8]
    responses_4a = rows[:,8:12]
    bytewise = bytewise_plot(responses_41[:1000,:],
                             responses_44[:1000,:],
                             responses_4a[:1000,:])
    asciifig = ascii_plot(responses_41, responses_44, responses_4a)
    plt.show()
