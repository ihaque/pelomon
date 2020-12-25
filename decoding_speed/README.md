# Regress Peloton speed

For more details, see the
[accompanying blog post](https://ihaque.org/posts/2020/12/25/pelomon-part-ib-computing-speed) on the [PeloMon project](https://ihaque.org/tag/pelomon.html).

`rider1.csv` and `rider2.csv` include training data from two
distinct riders showing power, speed, and cadence.

To get your own training data, you can download JSON dumps of your
rides using [PelotonData](https://github.com/cabird/PelotonData) or
by sniffing the Peloton web UI in your browser console. Convert these
JSON dumps to CSV using `peloton_json_to_csv.py`, which will reformat
the data and keep only unique data points. `regress_speed.py` can then
refit one- and two-piece polynomial fits to the data.
