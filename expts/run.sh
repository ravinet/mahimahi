#! /bin/bash

tcpdump -ttttt -n -r tcpdump_client_recv.pcap dst 128.30.77.13 and src 192.168.42.150 and port 50818  | cut -f 1 -d ' '  | cut -f 3 -d ':' | python mult.py > tcptrace.pps
mpshell 50 1000.pps tcptrace.pps 5 500.pps tcptrace.pps  "/usr/bin/wget --bind-address=100.64.0.2 http://128.30.77.91/rand -O /dev/null"
