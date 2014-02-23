#! /bin/bash

tcpdump -ttttt -n -r opt.pcap dst 100.64.0.2 and src 128.30.77.91 and port 80  | cut -f 1 -d ' '  | cut -f 3 -d ':' | python mult.py > tcprerun
