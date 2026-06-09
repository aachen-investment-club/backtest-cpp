|Date   |Dataset|Throughput (M Events/sec)|Ld1 Miss %|Branch Miss %|Ins per cycle|Max RSS (MB)|Notes|
|-------|-------|-------------------------|----------|-------------|-------------|------------|-----|
|2026-06-09:18:28|./data/NQ_sample.csv|0.5359|0.48|0.25|3.40|21.95|Heavily unoptimized still, profiling looks good because cpu is essentially doing a lot of unnecessary work (string operations) really fast, Events/sec target is 50M (~100x)|
