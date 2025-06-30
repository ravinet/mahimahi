#!/bin/bash

# === Configuration ===
TRACE_UP="../../traces/Verizon-LTE-short.up"
TRACE_DOWN="../../traces/Verizon-LTE-short.down"
L4S_TOS=1
DEFAULT_TIME=10 # seconds
DELAY=10 # milliseconds
OUTPUT_DIR="./output"
IPERF_TIME=${1:-$DEFAULT_TIME}
mkdir -p "$OUTPUT_DIR"

# === Start iperf3 servers on separate ports ===
echo "[*] Starting iperf3 servers on ports 5300 and 5301... waiting 5 seconds for ports to open up..."
iperf3 -s -p 5300 > "$OUTPUT_DIR/iperf_server_5300.log" 2>&1 &
iperf3 -s -p 5301 > "$OUTPUT_DIR/iperf_server_5301.log" 2>&1 &
sleep 5

# === Function to run test with given queue ===
run_test () {
  local QUEUE=$1
  local LOG_SUFFIX=$2

  echo ""
  echo "[*] Starting Mahimahi with uplink queue: $QUEUE for $IPERF_TIME seconds"

  mm-delay $DELAY mm-link --meter-all \
    --uplink-log=$OUTPUT_DIR/uplink_$LOG_SUFFIX.log \
    --downlink-log=$OUTPUT_DIR/downlink_$LOG_SUFFIX.log \
    --uplink-queue=$QUEUE \
    --uplink-queue-args="packets=100,interval=100,target=5" \
    "$TRACE_UP" "$TRACE_DOWN" -- bash -c "
      echo '[+] Running L4S-marked flow (ToS=1, port 5300) on $QUEUE...'
      iperf3 -c 10.0.0.1 -u -p 5300 -b 10M -t $IPERF_TIME --tos $L4S_TOS > $OUTPUT_DIR/iperf_l4s_$LOG_SUFFIX.log &

      echo '[+] Running Classic flow (default ToS, port 5301) on $QUEUE...'
      iperf3 -c 10.0.0.1 -u -p 5301 -b 10M -t $IPERF_TIME > $OUTPUT_DIR/iperf_classic_$LOG_SUFFIX.log &

      sleep 1
      echo '[*] Active iperf3 processes in sandbox:'
      ps aux | grep iperf3 | grep -v grep

      wait
    "
}

# === Run tests ===
echo "[*] Starting dualPI2 test in 5 seconds..."
sleep 5
run_test dualPI2 dualPI2
echo "[*] FINISHED dualPI2 test."

echo "[*] Starting CoDel test in 5 seconds..."
sleep 5
run_test codel codel
echo "[*] FINISHED CoDel test."

# === Cleanup ===
echo ""
echo "[*] Cleaning up iperf3 servers..."
pkill -f "iperf3 -s -p 5300"
pkill -f "iperf3 -s -p 5301"

# === Summary ===
echo ""
echo "[*] All tests complete. Log files saved in '$OUTPUT_DIR':"
ls "$OUTPUT_DIR"