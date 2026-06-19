# Backtesting Engine

![CI](https://github.com/aachen-investment-club/backtest-cpp/actions/workflows/ci.yml/badge.svg)

A high-performance, event-driven backtesting system for quantitative trading strategies built with C++20.  
`IMPORTANT:` The ToDos here aren't Jira tickets that "need to be fixed asap". This is meant to be an educational project to build real C++ / SWE skills.  
`TRY TO WRITE ALL CODE YOURSELF WITHOUT THE USE OF LLMs!`
You may use llms for explanations and hints but try to really understand these concepts and type out the code yourself.
  
## Build
You can build using either the `dev` (Debug, no LTO) or `release` (Optimized, LTO enabled) preset.

```bash
cmake --preset release
cmake --build --preset release
```

## Run
**Note:** As of now, and possibly subject to change, the executable relies on relative paths for data loading and must therefore be run from the **project root directory**.

```bash
./build/release/src/backtest
```

## Test
```bash
ctest --preset release
# (CI includes running ctest and the backtest binary)
```

## Benchmark
Benchmarks are built automatically alongside the project. Always run benchmarks using the `release` preset to ensure the compiler optimizations are active.

```bash
./build/release/benchmarks/system_benchmarks
```

## Performance
Run and profile the code via the analysis tool (currently linux only):
```bash
./build/release/performance/analyze
```  

The results and comparisons are written to the performance table in:
```bash
performance/PERFORMANCE.md
```

## Format
```bash
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```


## ToDos / Quant Developer Roadmap

The goal of this project is to build this rather standard backtester into a low-latency, high-performance backtester including zero-allocation hot paths and data-oriented design.  
  
(!) = High-Impact Optimization

### Phase 1: Quick Wins (Cache & Parsing)
- [IN PROGRESS] Refactor commission logic
- [X] (!) **Symbol Interning:** Replace `std::string` instrument keys with `uint32_t` IDs or Enums to remove string comparisons and allocations.
- [X] (!) **Contiguous Data Structures:** Replace `std::map<std::string, Bar>` with `std::vector<Bar>` indexed by Instrument ID to eliminate cache misses from pointer chasing.
- [X] **High-Precision Time:** Replace `time_t` (seconds) with `std::chrono::nanoseconds` or `int64_t` (nanoseconds since epoch).
- [ ] (!) **Cache-Line Alignment & Padding:** Align `Bar` and `Position` structs to 64-byte boundaries (`alignas(64)`) to optimize CPU L1 cache fetch and prevent false sharing.
- [ ] (!) **Fixed-Point Math:** Replace `double` for prices with `int64_t` fixed-point arithmetic (e.g., price * 10,000) to ensure deterministic logic and faster comparisons.

### Phase 2: Binary Data Mapping
- [ ] (!) **Memory Mapped I/O (`mmap`):** Map data files directly into process memory instead of performing user-space I/O copies.
- [ ] **Binary Data Format:** Implement a custom binary dump format to bypass CSV parsing entirely during the hot path.
- [ ] (!) **Zero-Copy Data Ingestion:** Refactor data passing so that the `Strategy` and `Portfolio` consume references/pointers directly from a memory-mapped buffer, eliminating data copies.
- [ ] **Custom Memory Pool:** Implement `std::pmr::memory_resource` for any unavoidable dynamic allocations during execution.

### Phase 3: Advanced Systems & LOB (Complex Features)
- [ ] **CPU Pinning / Thread Affinity:** Isolate the backtest thread to a specific CPU core to prevent context switching.
- [ ] **Structure of Arrays (SoA):** Refactor data layouts (e.g., separating Open, High, Low, Close arrays) to maximize SIMD vectorization for indicator math.
- [ ] **L2 Limit Order Book (LOB):** Move beyond OHLC bars to full tick-data and order book reconstruction.
- [ ] **Latency Modeling:** Introduce simulated network delay (nanoseconds) between signal generation and order execution.
- [ ] **Basic Matching Engine:** Implement FIFO queue-position logic for limit order execution.

### Phase 4: Extended Functionality (Integration & Scaling)
- [ ] **Python Bindings:** Use `pybind11` to expose the C++ engine to Python for easier research and visualization.
- [ ] **API Connectivity:** Implement WebSocket/FIX protocol handlers for live trading integration (Binance, Interactive Brokers, etc.).
- [ ] **Multi-Asset Support:** Scale the engine to handle thousands of concurrent symbols with columnar storage.
- [ ] **Web Dashboard:** Build a real-time visualization dashboard (React/TypeScript) to monitor backtest progress and equity curves.
  
### Current Components
- **DataHandler**: CSV loading and bar iteration
- **Portfolio**: Position tracking, P&L calculation, order execution
- **Strategy**: Base class for trading strategies (SMA Crossover implemented)
- **Types**: Core domain objects (Bar, Order, Signal, Trade, Position)

### Event Flow
```
Market Data → Strategy → Signal → Order → Portfolio → Execution
```
