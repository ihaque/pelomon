# Convert Peloton JSON dumps to CSV
# Part of the PeloMon project: https://github.com/ihaque/pelomon
#
# Copyright 2020 Imran S Haque (imran@ihaque.org)
# Licensed under the CC-NY-NC 4.0 license
# (https://creativecommons.org/licenses/by-nc/4.0/).

import json
import sys

import pandas as pd

def load_peloton_json(filename):
    with open(filename) as jf:
        data = json.load(jf)
    values = {}
    try:
        data['metrics']
    except:
        print(filename, data.keys())
    for metric in data['metrics']:
        if metric['slug'] == 'output':
            values['power'] = list(map(float,metric['values']))
        elif metric['slug'] == 'cadence':
            values['cadence'] = list(map(float,metric['values']))
        elif metric['slug'] == 'resistance':
            values['resistance'] = list(map(float,metric['values']))
        elif metric['slug'] == 'speed':
            assert metric['display_unit'] == 'mph'
            values['speed'] = list(map(float,metric['values']))
    return pd.DataFrame.from_dict(values)

def load_data(jsons):
    df = pd.concat([load_peloton_json(fn) for fn in jsons])
    # Drop oversampled points
    return df.drop_duplicates(subset=['speed', 'power'])

if __name__ == '__main__':
    jsons = sys.argv[1:]
    load_data(jsons).to_csv(sys.stdout, index=False)
