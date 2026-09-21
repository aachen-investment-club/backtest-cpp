#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>
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
constexpr bool DEBUG = false;
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

    uint32_t nq_id = symDict.get_id("NQ_sample.csv"); // NQ_sample
    std::cout << "NQ_ID from Dictionary: " << nq_id << "\n";

    SMACrossover strategy(nq_id, 10, 30);

    // -------------------------------------------------
    // Strategy warm-up (SMA lookback)
    // -------------------------------------------------
    std::vector<Bar> historicalData;

    for (int i = 0; i < 30 && dataHandler.hasMoreData(); ++i) {
        historicalData.push_back(dataHandler.getCurrentBars()[0]);
    }
    strategy.onInit(historicalData);
    std::cout << "Starting backtest..." << "\n";

    // -------------------------------------------------
    // Equity curve storage
    // -------------------------------------------------
    std::vector<EquityPoint> equityCurve;
    equityCurve.reserve(100'000);  // avoid reallocations

    std::deque<Order> openOrders;
    openOrders.clear();

    int barCount = 0;

    // -------------------------------------------------
    // Main backtest loop
    // -------------------------------------------------
    std::vector<Signal> signals;
    std::vector<Bar> barVector(2); 
    Bar bar;
    auto start = std::chrono::steady_clock::now();  // Timing the hot loop
    while (dataHandler.hasMoreData()) {
        signals.clear();
        barVector = dataHandler.getNextBars();
        bar = barVector.at(1);
        //std::cout << dataHandler.getNextBars()[] << "\n";

        for (Order& order : openOrders) {
            order.price = bar.open;
            portfolio.executeOrder(order, true);
            // std::cerr << "Executing order. \n";
            // std::cerr << order.price << "\n"; 
            // std::cerr << order.quantity << "\n"; 

            break;
        }
        openOrders.clear();

        strategy.onBar(bar, signals);

        openOrders.emplace_back(strategy.generateOrders(signals, bar, portfolio.getCurrentPositions())[nq_id]);

        if (DEBUG) {
            std::cout << "DEBUG: equity: " << priceIntToDouble(portfolio.getTotalEquity({bar}))
                        << "\n";

            auto it = portfolio.getCurrentPositions().find(nq_id);
            std::cout << "INFO | Total Positions After: "
                        << (it != portfolio.getCurrentPositions().end() ? it->second.quantity : 0)
                        << "\n";
    
            std::cout << "DEBUG: Logged time: " << bar.time << "\n";
            std::cout << "DEBUG: Logged Equity: " << portfolio.getTotalEquity({bar}) << "\n";

            std::cout << "----------------------------------------------" << "\n";
        }

        // Record equity every bar (CRITICAL)
        equityCurve.push_back({.time=bar.time, .equity=portfolio.getTotalEquity(barVector)});
        ++barCount;
    }
    auto end = std::chrono::steady_clock::now();  // End timing at end of hot loop
    std::chrono::duration<double> elapsed_seconds = end - start;

    // -------------------------------------------------
    // Final liquidation
    // -------------------------------------------------
    std::vector<Bar> finalBars = dataHandler.getCurrentBars();
    portfolio.closeAllPositions(finalBars);

    int64_t barTime = 0;  // find time of the first bar in finalBars
    for (auto& curBar : finalBars) {
        if (curBar.time != 0) {
            barTime = curBar.time;
            break;
        }
    }
    equityCurve.push_back({.time=barTime, .equity=portfolio.getTotalEquity(finalBars)});  // TODO !!!

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