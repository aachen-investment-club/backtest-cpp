#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>
#if defined(__linux__)
#include <sys/resource.h>
#endif

#include "backtest-cpp/data.h"
#include "backtest-cpp/performance.h"
#include "backtest-cpp/portfolio.h"
#include "backtest-cpp/strategies/smacrossover.h"
#include "backtest-cpp/symbol_dictionary.h"

#define DEBUG false

int main() {
    auto start = std::chrono::steady_clock::now();

    std::cout << "=== Backtesting Engine ===" << std::endl;

    // -------------------------------------------------
    // Initialization
    // -------------------------------------------------
    symbol_dictionary symDict;
    DataHandler dataHandler(symDict);
    Portfolio portfolio({.initialCash = 100'000.0, .commission = 2.7, .leverage = 1.0});
    
    uint32_t nq_id = symDict.get_id("NQ"); // each symbol should be added to the dictionary before doing LoadCSV
        
    SMACrossover strategy(nq_id, 10, 30);

    dataHandler.loadCSV("./data/NQ_sample.csv", nq_id);
    // dataHandler.loadAllCSVs("./data"); // TODO

    // -------------------------------------------------
    // Strategy warm-up (SMA lookback)
    // -------------------------------------------------
    std::vector<std::vector<Bar>> historicalData;

    for (int i = 0; i < 30 && dataHandler.hasMoreData(); ++i) {
        historicalData.push_back(dataHandler.getNextBars());
    }
    strategy.onInit(historicalData);

    std::cout << "Starting backtest..." << std::endl;

    // -------------------------------------------------
    // Equity curve storage
    // -------------------------------------------------
    std::vector<EquityPoint> equityCurve;
    equityCurve.reserve(100000);  // avoid reallocations

    int barCount = 0;

    // -------------------------------------------------
    // Main backtest loop
    // -------------------------------------------------
    while (dataHandler.hasMoreData()) {
        std::vector<Bar> bars = dataHandler.getNextBars();

        auto posIt = portfolio.getCurrentPositions().find(nq_id);
        if (posIt != portfolio.getCurrentPositions().end() && bars[nq_id].symbol_id == 0) {
            std::cerr << "BUG: Have NQ position but bars doesn't contain NQ at bar " << barCount
                      << std::endl;
        }

        std::map<uint32_t, std::optional<Signal>> signalMap =
            strategy.onBars(bars, portfolio.getCurrentPositions());

        for (const auto& [symbol, signal] : signalMap) {
            if (signal.has_value()) {
                Order order = strategy.generateOrder(signal.value(), bars[symbol], 10'000,
                                                     portfolio.getCurrentPositions());
                if (DEBUG) {
                    std::cout << "Order at bar " << barCount << ": "
                              << (signal->type == SignalType::BUY ? "BUY " : "SELL ")
                              << order.quantity << " @ " << bars[symbol].close << std::endl;

                    std::cout << "INFO | Unrealized PnL : " << portfolio.getUnrealizedPnL(bars)
                              << " | Realized PnL : " << portfolio.getRealizedPnL() << std::endl;

                    std::cout << "INFO | Total Equity Before: " << portfolio.getTotalEquity(bars)
                              << std::endl;
                }

                portfolio.executeOrder(order, true);

                if (DEBUG) {
                    std::cout << "INFO | Total Equity After: " << std::setprecision(7)
                              << portfolio.getTotalEquity(bars) << std::endl;

                    std::cout << "DEBUG: equity: " << portfolio.getTotalEquity(bars) << std::endl;

                    auto it = portfolio.getCurrentPositions().find(nq_id);
                    std::cout << "INFO | Total Positions After: "
                              << (it != portfolio.getCurrentPositions().end() ? it->second.quantity
                                                                              : 0)
                              << std::endl;

                    std::cout << "----------------------------------------------" << std::endl;
                }
            }
        }
        // std::cout << "DEBUG: Logged time: " << bars.begin()->second.time << std::endl;
        // std::cout << "DEBUG: Logged Equity: " << portfolio.getTotalEquity(bars) << std::endl;

        // Record equity every bar (CRITICAL)
        int64_t barTime = 0; // find time of the first bar in bars
        for(auto &curBar : bars) {  // necessary to search until dataHandler::synchronize is implemented
            if(curBar.symbol_id != 0) {
                barTime = curBar.time;
                break;
            }
        }
        equityCurve.push_back({barTime, portfolio.getTotalEquity(bars)});

        ++barCount;
    }

    // -------------------------------------------------
    // Final liquidation
    // -------------------------------------------------
    std::vector<Bar> finalBars = dataHandler.getCurrentBars();
    portfolio.closeAllPositions(finalBars);
    
    int64_t barTime = 0; // find time of the first bar in finalBars
    for(auto &curBar : finalBars) {  // necessary to search until dataHandler::synchronize is implemented
        if(curBar.symbol_id != 0) {
            barTime = curBar.time;
            break;
        }
    }
    equityCurve.push_back({barTime, portfolio.getTotalEquity(finalBars)});   // TODO !!!

    // -------------------------------------------------
    // Backtest summary
    // -------------------------------------------------
    std::cout << "\n=== Backtest Complete ===" << std::endl;
    std::cout << "Bars processed : " << barCount << std::endl;
    std::cout << "Trades         : " << portfolio.getAllTrades().size() << std::endl;
    std::cout << "Realized PnL   : " << portfolio.getRealizedPnL() << std::endl;
    std::cout << "Final Equity   : " << portfolio.getTotalEquity(finalBars) << std::endl;

    std::cout << "\n=== Strategy Performance Statistics ===" << std::endl;

    double annReturn = Performance::annualizedReturn(equityCurve, Frequency::MINUTE);

    double annVol = Performance::annualizedVolatility(equityCurve, Frequency::MINUTE);

    double sharpe = Performance::sharpeRatio(equityCurve, Frequency::MINUTE, 0.0);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Annualized Return : " << annReturn * 100 << " %" << std::endl;
    std::cout << "Annualized Vol    : " << annVol * 100 << " %" << std::endl;
    std::cout << "Sharpe Ratio      : " << sharpe << std::endl;

    std::cout << "\n=== System Performance Statistics ===" << std::endl;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

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
