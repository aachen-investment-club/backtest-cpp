# Performance Tracking
Since this is a high performance backtester, we need to properly measure performance.
The performance tracking script takes the guesswork out and validates whether a new change actually improves performance or not (or, just as important, protects us from including bad code that would diminish performance).
The performance analysis script and suggested perf analysis workflow is `Linux only for now`.  

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
The execution is pinned to a performance core (Core 2 with 4.8GHz max) and the CPU governor is set to performance.
The performance script is ran via
```bash
sudo cpupower frequency-set -g performance
sudo cpupower frequency-set -d 4.8GHz -u 4.8GHz  
taskset -c 2 ./build/release/performance/analyze
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
taskset -c 2 perf record --call-graph dwarf ./build/release/src/backtest
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