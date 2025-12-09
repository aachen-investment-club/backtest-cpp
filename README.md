# Backtesting Engine
A high-performance, event-driven backtesting system for quantitative trading strategies built with C++17.
  
## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Run
```bash
./backtest
```

## Test
```bash
ctest
```

## Format
```bash
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## ToDos
### Code Quality
- [ ] Refactor commission logic
- [ ] More flexible CSV parsing logic
- [ ] More rigorous testing
- [ ] Add integration tests
- [ ] Improve error handling

### Functionality
- [ ] Add performance.h/cpp (Annualized Sharpe, Profit Factor, Max Drawdown, etc.)
- [ ] Add bid-ask spread modeling
- [ ] Multi-asset support (multiple stocks)
- [ ] Complex derivatives support (options pricing)
- [ ] API connectivity (live trading)
- [ ] ML strategy support + feature engineering
- [ ] GUI (real-time visualization)

### Performance Optimization
- [ ] Hot path analysis / profiling
- [ ] Compile with optimization flags (`-O3 -march=native`)
- [ ] Columnar storage (for multi-asset)
- [ ] Multithreading (parallel asset processing)
- [ ] Implement event queue architecture (for API connectivity)
  
### Current Components
- **DataHandler**: CSV loading and bar iteration
- **Portfolio**: Position tracking, P&L calculation, order execution
- **Strategy**: Base class for trading strategies (SMA Crossover implemented)
- **Types**: Core domain objects (Bar, Order, Signal, Trade, Position)

### Event Flow
```
Market Data → Strategy → Signal → Order → Portfolio → Execution
```
