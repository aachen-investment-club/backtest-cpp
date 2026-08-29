#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>
#include <unordered_map>
#if defined(__linux__)
#include <sys/resource.h>  // for linux native performance tracking
#endif

#include <filesystem>

#include "backtest-cpp/data.h"
#include "backtest-cpp/performance.h"
#include "backtest-cpp/portfolio.h"
#include "backtest-cpp/strategies/smacrossover.h"
#include "backtest-cpp/symbol_dictionary.h"

// -------------------------------------------------
// BACKTEST PARAMETERS
// -------------------------------------------------
#define DEBUG false
inline const std::string DATA_DIRECTORY{"./data/used_data"};
// -------------------------------------------------

int main() {
    
    std::cout << "=== Backtesting Engine ===" << "\n";
    
    // -------------------------------------------------
    // Initialization
    // -------------------------------------------------
    // 1. Initialization
    symbol_dictionary symDict;
    DataHandler dataHandler;
    Portfolio portfolio({.initialCash = 100'000.0, .commission = 2.7, .leverage = 1.0});
    
    // 2. LOAD DATA FIRST!
    dataHandler.loadAllCSVs(DATA_DIRECTORY, symDict, "string");
    
    uint32_t nq_id = symDict.get_id("NQ_sample.csv");
    std::cout << "NQ_ID from Dictionary: " << nq_id << "\n";
    
    SMACrossover strategy(nq_id, 10, 30);
    
    // -------------------------------------------------
    // Strategy warm-up (SMA lookback)
    // -------------------------------------------------
    std::vector<std::vector<Bar>> historicalData;
    
    for (int i = 0; i < 30 && dataHandler.hasMoreData(); ++i) {
        historicalData.push_back(dataHandler.getNextBars());
    }
    strategy.onInit(historicalData);
    std::cout << "Starting backtest..." << "\n";
    
    // -------------------------------------------------
    // Equity curve storage
    // -------------------------------------------------
    std::vector<EquityPoint> equityCurve;
    equityCurve.reserve(100000);  // avoid reallocations
    
    std::deque<Order> openOrders;
    
    int barCount = 0;
    
    // -------------------------------------------------
    // Main backtest loop
    // -------------------------------------------------
    auto start = std::chrono::steady_clock::now(); // Timing the hot loop
    while (dataHandler.hasMoreData()) {
        std::vector<Bar>& bars = dataHandler.getNextBars();

        // Execute open orders
        for (Order &order : openOrders) {
            for (const auto &bar : bars) {
                if (bar.symbol_id == order.symbol_id) {
                    order.price = bar.open;
                    portfolio.executeOrder(order, true);
                    break;
                }
            }
        }
        openOrders.clear();

        std::unordered_map<uint32_t, std::optional<Signal>> signalMap =
            strategy.onBars(bars, portfolio.getCurrentPositions());

        for (const auto &[symbol, signal] : signalMap) {
            if (signal.has_value()) {
                // Order order = strategy.generateOrder(signal.value(), bars[symbol], 10'000,
                //                                      portfolio.getCurrentPositions());

                openOrders.emplace_back(strategy.generateOrder(
                    signal.value(), bars[symbol], 10'000,
                    portfolio.getCurrentPositions()));  // Using rvalue like a real nigga

                if (DEBUG) {
                    std::cout << "Order at bar " << barCount << ": "
                              << (signal->type == SignalType::BUY ? "BUY " : "SELL ") << " @ "
                              << priceIntToDouble(bars[symbol].open) << "\n";
                    std::cout << "INFO | Unrealized PnL : "
                              << priceIntToDouble(portfolio.getUnrealizedPnL(bars))
                              << " | Realized PnL : "
                              << priceIntToDouble(portfolio.getRealizedPnL()) << "\n";

                    std::cout << "INFO | Total Equity Before: "
                              << priceIntToDouble(portfolio.getTotalEquity(bars)) << "\n";
                }

                // portfolio.executeOrder(order, true);

                if (DEBUG) {
                    std::cout << "INFO | Total Equity After: " << std::setprecision(7)
                              << priceIntToDouble(portfolio.getTotalEquity(bars)) << "\n";

                    std::cout << "DEBUG: equity: "
                              << priceIntToDouble(portfolio.getTotalEquity(bars)) << "\n";

                    auto it = portfolio.getCurrentPositions().find(nq_id);
                    std::cout << "INFO | Total Positions After: "
                              << (it != portfolio.getCurrentPositions().end() ? it->second.quantity
                                                                              : 0)
                              << "\n";

                    std::cout << "----------------------------------------------" << "\n";
                }
            }
        }

        // std::cout << "DEBUG: Logged time: " << bars.begin()->second.time << "\n";
        // std::cout << "DEBUG: Logged Equity: " << portfolio.getTotalEquity(bars) << "\n";

        // Record equity every bar (CRITICAL)
        int64_t barTime = 0;
        for (const auto &curBar : bars) {
            if (curBar.time > barTime) {
                barTime = curBar.time;
            }
        }

        if (barTime > 0) {
            equityCurve.push_back({barTime, portfolio.getTotalEquity(bars)});
        }
        ++barCount;
    }
    auto end = std::chrono::steady_clock::now(); // End timing at end of hot loop
    std::chrono::duration<double> elapsed_seconds = end - start;

    // -------------------------------------------------
    // Final liquidation
    // -------------------------------------------------
    std::vector<Bar> finalBars = dataHandler.getCurrentBars();
    portfolio.closeAllPositions(finalBars);

    int64_t barTime = 0;  // find time of the first bar in finalBars
    for (auto &curBar : finalBars) {
        if (curBar.time != 0) {
            barTime = curBar.time;
            break;
        }
    }
    equityCurve.push_back({barTime, portfolio.getTotalEquity(finalBars)});  // TODO !!!

    // -------------------------------------------------
    // Backtest summary
    // -------------------------------------------------
    std::cout << "\n=== Backtest Complete ===" << "\n";
    std::cout << "Bars processed : " << barCount << "\n";
    std::cout << "Trades         : " << portfolio.getAllTrades().size() << "\n";
    std::cout << "Realized PnL   : " << priceIntToDouble(portfolio.getRealizedPnL()) << "\n";
    std::cout << "Final Equity   : " << priceIntToDouble(portfolio.getTotalEquity(finalBars))
              << "\n";

    std::cout << "\n=== Strategy Performance Statistics ===" << "\n";

    double annReturn = Performance::annualizedReturn(equityCurve, Frequency::MINUTE);

    double annVol = Performance::annualizedVolatility(equityCurve, Frequency::MINUTE);

    double sharpe = Performance::sharpeRatio(equityCurve, Frequency::MINUTE, 0.0);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Annualized Return : " << annReturn * 100 << " %" << "\n";
    std::cout << "Annualized Vol    : " << annVol * 100 << " %" << "\n";
    std::cout << "Sharpe Ratio      : " << sharpe << "\n";

    std::cout << "\n=== System Performance Statistics ===" << "\n";

    double throughput =
        static_cast<double>(dataHandler.size()) / elapsed_seconds.count() / 1'000'000.0;
    std::cout << "Elapsed Time: : " << elapsed_seconds.count() * 1000 << " ms\n";
    std::cout << "Throughput: " << throughput << "M Events/sec\n";

#if defined(__linux__)
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double max_rss_mb = static_cast<double>(usage.ru_maxrss) / 1024.0;
        std::cout << "Max Resident Size: " << max_rss_mb << "MB\n";
    } else {
        std::cerr << "Failed to get memory usage.\n";
    }
#endif

    return 0;
}