|Date   |Dataset|Throughput (M Events/sec)|Ld1 Miss %|Branch Miss %|Ins per cycle|Max RSS (MB)|Notes|
|-------|-------|-------------------------|----------|-------------|-------------|------------|-----|
|2026-06-09:18:28|./data/NQ_sample.csv|0.5359|0.48|0.25|3.40|21.95|Heavily unoptimized still, profiling looks good because cpu is essentially doing a lot of unnecessary work (string operations) really fast, Events/sec target is 50M (~100x)|
|2026-06-19:18:32|./data/NQ_sample.csv|0.5452|0.29|0.25|3.39|20.91|Run with contiguous vector to store bars instead of map, significant expected reduction in L1d Miss % is present|
|2026-07-03:23:33|./data/NQ_sample.csv|10.5596|2.88|0.43|3.06|12.34|New mmapped binary data gives enormous 18x performance gain in throughput. Memory footprint has also been almost halved. The other metrics are drastically worse from a % standpoint but in an absolute sense still very good imo.|
|2026-07-13:16:56|./data/NQ_sample.csv|10.7216|0.18|0.45|3.52|12.51|First run with proper performance test pipeline that includes: core pinning, warmup runs, averaged results over N runs (here 20 runs)
Throughput and max RSS are unchanged, while the previous spike in L1d miss % and Lower IPC were false signals|
|2026-07-13:20:24|./data/NQ_sample.csv|11.0903|0.17|0.46|3.61|12.50|Minor performance gain from single pass volatility calculation. std::log has not been the bottleneck for volatility calculation|
