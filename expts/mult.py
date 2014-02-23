#! /usr/bin/python
import sys
fh=sys.stdin
for line in fh.readlines():
  ts = float(line.strip().split()[0])
  print int(round(ts*1000.0))
