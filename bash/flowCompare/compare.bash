#!/bin/bash

# === Configuration ===
TRACE_UP="../../traces/Verizon-LTE-short.up"
TRACE_DOWN="../../traces/Verizon-LTE-short.down"
L4S_TOS=1
DEFAULT_TIME=30
DELAY=10
IPERF_TIME=${1:-$DEFAULT_TIME}

# === Start iperf3 servers ===
echo "[*] Starting iperf3 servers on ports 5300 and 5301..."
iperf3 -s -p 5300 &
iperf3 -s -p 5301 &
sleep 2

# === Run a test and show graph ===
run_test () {
  local QUEUE=$1
  echo ""
  echo "[*] Running $QUEUE queue with built-in live graphs for $IPERF_TIME seconds"

  mm-delay $DELAY mm-link --meter-all \
    --uplink-queue=$QUEUE \
    --uplink-queue-args="packets=100,interval=100,target=5" \
    "$TRACE_UP" "$TRACE_DOWN" -- bash -c "
      echo '[+] Starting iperf3 L4S (port 5300)...'
      iperf3 -c 10.0.0.1 -u -p 5300 -b 5M -t $IPERF_TIME --tos $L4S_TOS &
      
      echo '[+] Starting iperf3 Classic (port 5301)...'
      iperf3 -c 10.0.0.1 -u -p 5301 -b 5M -t $IPERF_TIME &

      echo '[*] Running ps to check iperf3 processes:'
      ps aux | grep iperf3 | grep -v grep

      wait
    "
}

# === Run Tests ===
# Switch between the two blocks or run both at the same time. 

# echo "[*] Starting dualPI2 test in 5 seconds..."
# sleep 5
# run_test dualPI2
# echo "[*] FINISHED dualPI2 test."

echo "[*] Starting CoDel test in 5 seconds..."
sleep 5
run_test codel
echo "[*] FINISHED CoDel test."

# === Cleanup ===
echo "[*] Cleaning up iperf3 servers..."
pkill -f "iperf3 -s -p 5300"
pkill -f "iperf3 -s -p 5301"
