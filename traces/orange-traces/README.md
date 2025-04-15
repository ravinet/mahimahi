# Traces on Orange 4G commercial network

These trace files represent the time-varying capacity of Orange France 4G cellular
networks as experienced by a mobile user. They were recorded using the
"Saturator" tool described in the following [research paper](https://www.usenix.org/conference/nsdi13/technical-sessions/presentation/winstein).


The measurements were performed on Orange 4G network in Lannion (France) in January 2022 and each capture lasts approximatively 10 minutes. We choosed 5 static configurations based on the average downlink throughput.  We also perform a measurement on mobility, on the highway between Guingamp and Lannion at 110 km/h.
The table below summarizes the traces files and their measurements conditions.

| File name | Average downlink throughput (Mbps) | Location 
|:---|:---:|:---|
| orange-4G-traces-E-\*.pps	| 220 |	Orange Lannion |	
| orange-4G-traces-D-\*.pps	| 160 |	Orange Lannion |	
| orange-4G-traces-C-\*.pps	| 120 |	Lannion (Brélévenez) |	
| orange-4G-traces-B-\*.pps	| 80  |	Lannion (Brélévenez) |	
| orange-4G-traces-A-\*.pps	| 40  |	Plemeur Bodou |
| orange-4G-traces-highway-\*.pps	| 45  |	Guingamp – Lannion |	

where ```*``` can be *up* or *down*.

Each line gives a timestamp in milliseconds (from the beginning of the
trace) and represents an opportunity for one 1500-byte packet to be
drained from the bottleneck queue and cross the link. If more than one
MTU-sized packet can be transmitted in a particular millisecond, the
same timestamp is repeated on multiple lines.


## Usage:

For more information about the usage, refer to [Mahimahi](http://mahimahi.mit.edu/).

## Further information

More information about the traces can be found at this [link](https://cloud-gaming-traces.lhs.loria.fr/cellular.html).