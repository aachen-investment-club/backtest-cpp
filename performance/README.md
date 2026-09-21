# Performance Tracking
Since this is a high performance backtester, we need to properly measure performance.
The performance tracking script takes the guesswork out and validates whether a new change actually improves performance or not (or, just as important, protects us from including bad code that would diminish performance).
The performance analysis script and suggested perf analysis workflow is `Linux only for now` and the code, as written here, is `specific to the benchmark machine` (e.g. what core is isolated, set core frequency, etc.).  

## Benchmarking Hardware
Currently using my Linux Laptop with the following cpu specs:
```
Intel(R) Core(TM) Ultra 7 155H
 CPU op-mode(s):            32-bit, 64-bit
CPU(s):                      22
  On-line CPU(s) list:       0-21
  Model name:                Intel(R) Core(TM) Ultra 7 155H
    CPU family:              6
    Thread(s) per core:      2
    Core(s) per socket:      16
    Socket(s):               1
    Stepping:                4
    CPU max MHz:             4800.0000
```

## Benchmarking approach
The backtest run over the same sample strategy and dataset is repeated N times (default 20) and also a few discarded warmup runs. 
The execution is pinned to a performance core (Core 3 with 4.8GHz max) and the CPU governor is set to performance.
Cores 3 and 4 are SMT siblings on the benchmark machine. I isolate cpu 3 and 4, then disable cpu 4 to use core 3 to make full use of the 4.8GHz performance core.

## Quick Benchmark
```bash
sudo cpupower frequency-set -d 4.8GHz -u 4.8GHz  
taskset -c 3 ./build/release/performance/analyze
```  

## Full precise Benchmark
### Set isolation (Reboots system)
```bash
sudo kernelstub -a "isolcpus=domain,managed_irq,3,4 nohz_full=3,4 rcu_nocbs=3,4"
sudo reboot
```
  
### Setup benchmark
```bash
system76-power profile performance
echo 0 | sudo tee /sys/devices/system/cpu/cpu4/online
sudo cpupower -c 3 idle-set -D 0 
sudo cpupower -c 3 frequency-set -g performance
sudo cpupower -c 3 frequency-set -d 4.8GHz -u 4.8GHz 
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
grep . /sys/devices/system/cpu/cpufreq/policy3/scaling_{min,max}_freq
```
  
### Run benchmark
```bash
sudo chrt -f 80 taskset -c 3 ./build/release/performance/analyze
```

### Teardown
```bash
sudo cpupower -c 3 frequency-set -d 400MHz -u 4.8GHz
sudo cpupower -c 3 idle-set -E
echo 1 | sudo tee /sys/devices/system/cpu/cpu4/online
system76-power profile balanced
```
  
## Metrics tracked
### Metadata
* Date
* Dataset

### Performance Metrics
* Throughput: Measured in million events / second. This is THE MAIN METRIC we optimize for, we want our backtester to process as many events as fast as possible, making maximum use of the hardware.
* L1d miss %: Level 1 data cache misses. We want this to be as low as possible so the cpu can efficiently work with the available data.
* Branch miss %: How well can the CPU predict what branch will be executed next? A false prediction causes a costly pipeline stall. 
Thus, for maximum performance, we want to keep the Branch miss % as low as possible.
* IPC (Instructions per Cycle): How many instructions are executed in one cpu cycle of the backtester running. The higher the better.
* Max RSS: Maximum Resident Set Size: The maximum amount of memory used by the running backtesting process at any time, measured in MB. We want to keep the memory footprint low. This is not that critical for small runs (say on a 100k line dataset) but becomes an increasing concern with larger, more complex datasets.

## Further analysis via PERF
```bash
taskset -c 3 perf record --call-graph dwarf ./build/release/src/backtest
perf report
```

## FLamegraph (using FlameGraph repository)
```
# Inside FlameGraph Repository
# Current out.perf run inside the FlameGraph directory
perf script > out.perf
./stackcollapse-perf.pl out.perf > out.folded
./flamegraph.pl out.folded > flame.svg
```