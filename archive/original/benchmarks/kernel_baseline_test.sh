#!/bin/bash

# kernel_baseline_test.sh
# Run kernel performance benchmarks and store outputs

# Output folder
RESULTS="${RESULTS:-kernel_default}"

mkdir -p "$RESULTS"
echo "[+] Saving results to $RESULTS"

# Save kernel info
uname -a > "$RESULTS/kernel_info.txt"

# --- Sysbench CPU Test ---
echo "[+] Running sysbench CPU test..."
sysbench cpu --cpu-max-prime=50000 run > "$RESULTS/sysbench_cpu.txt"

# --- Stress-ng CPU Test ---
echo "[+] Running stress-ng..."
stress-ng --cpu 4 --timeout 30s --metrics-brief > "$RESULTS/stressng_cpu.txt"

# --- Perf Scheduling Latency Test ---
echo "[+] Running perf sched latency..."
sudo perf sched record -o perf.data -- sleep 5
sudo timeout 10 perf sched latency -i perf.data > "$RESULTS/perf_sched_latency.txt" || echo "Perf failed" >> "$RESULTS/perf_sched_latency.txt"
rm -f perf.data

# --- Sysbench File I/O Test ---
echo "[+] Running sysbench file I/O..."
sysbench fileio --file-test-mode=rndrw prepare > /dev/null
sysbench fileio --file-test-mode=rndrw --max-time=30 --max-requests=0 run > "$RESULTS/sysbench_fileio.txt"
sysbench fileio cleanup > /dev/null

# --- CPU Fairness Test using `yes` ---
echo "[+] Starting CPU fairness test..."
(for i in {1..4}; do yes > /dev/null & echo $!; done) > "$RESULTS/yes_pids.txt"
sleep 10
top -b -n 1 > "$RESULTS/top_snapshot.txt"
xargs kill < "$RESULTS/yes_pids.txt"

echo "[✓] All tests complete. Results saved to $RESULTS"
