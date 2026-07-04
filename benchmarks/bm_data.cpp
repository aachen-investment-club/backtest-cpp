#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

#include "backtest-cpp/data.h"
#include "backtest-cpp/types.h"
#include "backtest-cpp/utils.h"

static void CreateDummyCSV(const std::string& filepath, int64_t numRows) {
    std::ofstream file(filepath);
    file << "datetime,open,high,low,close,volume\n";

    // Define base time and step in explicitly typed 64-bit nanoseconds
    const int64_t base_timestamp_ns = 1609459200000000000LL;

    const int64_t hour_in_ns = 3600000000000LL;

    for (int64_t i = 0; i < numRows; i++) {
        double basePrice = 3700.0 + static_cast<double>(i % 100);

        // Write the 64-bit nanosecond integer to the file
        file << (base_timestamp_ns + i * hour_in_ns) << "," << basePrice << ","
             << (basePrice + 10.0) << "," << (basePrice - 10.0) << "," << (basePrice + 5.0) << ","
             << (100000 + (i % 1000) * 1000) << "\n";
    }
}

// Benchmark retrieving bars sequentially
static void BM_DataHandler_GetNextBars(benchmark::State& state) {
    const int64_t numRows = state.range(0);
    const std::string filepath = "bm_temp_get_" + std::to_string(numRows) + ".csv";
    CreateDummyCSV(filepath, numRows);

    symbol_dictionary symDict;
    DataHandler handler;
    handler.loadCSV(filepath, 0, "nanosecond");

    for (auto _ : state) {
        state.PauseTiming();
        handler.reset();
        state.ResumeTiming();

        while (handler.hasMoreData()) {
            auto bars = handler.getNextBars();
            benchmark::DoNotOptimize(bars);
        }
    }

    std::remove(filepath.c_str());
    state.SetItemsProcessed(state.iterations() * numRows);
    state.SetComplexityN(numRows);
}

BENCHMARK(BM_DataHandler_GetNextBars)->RangeMultiplier(10)->Range(100, 100000)->Complexity();
