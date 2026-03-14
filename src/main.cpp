#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>

#include "../strategies/smacrossover.h"
#include "data.h"
#include "performance.h"
#include "portfolio.h"

int main() {
    std::cout << "=== Backtesting Engine ===" << std::endl;

    // -------------------------------------------------
    // Initialization
    // -------------------------------------------------
    DataHandler dataHandler;
    Portfolio portfolio({.initialCash = 100'000.0, .commission = 2.7, .leverage = 1.0});

    SMACrossover strategy(10, 30);

    //dataHandler.loadCSV("../data/Mini.csv", "NQ");
    dataHandler.loadCSV("../data/NQ_sample.csv", "NQ");
    //dataHandler.loadAllCSVs("../data");

    
    // -------------------------------------------------
    // Strategy warm-up (SMA lookback)
    // -------------------------------------------------
    std::vector<std::map<std::string, Bar>> historicalData;

    for (int i = 0; i < 30 && dataHandler.hasMoreData(); ++i) {
        std::cout << "Added Bar" << std::endl;
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
        std::map<std::string, Bar> bars = dataHandler.getNextBars();

    auto posIt = portfolio.getCurrentPositions().find("NQ");
    if (posIt != portfolio.getCurrentPositions().end() && bars.find("NQ") == bars.end()) {
        std::cerr << "BUG: Have NQ position but bars doesn't contain NQ at bar " 
                  << barCount << std::endl;
    }

        std::map<std::string, std::optional<Signal>> signalMap =
            strategy.onBars(bars, portfolio.getCurrentPositions());

        for(const auto& [symbol, signal] : signalMap) {
            if (signal.has_value()) {
                Order order = strategy.generateOrder(
                    signal.value(),
                    bars[symbol],
                    10'000,
                    portfolio.getCurrentPositions());

                std::cout << "Order at bar " << barCount << ": "
                        << (signal->type == SignalType::BUY ? "BUY " : "SELL ")
                        << order.quantity << " @ " << bars[symbol].close << std::endl;

                std::cout << "INFO | Unrealized PnL : "
                        << portfolio.getUnrealizedPnL(bars)
                        << " | Realized PnL : "
                        << portfolio.getRealizedPnL() << std::endl;

                std::cout << "INFO | Total Equity Before: "
                        << portfolio.getTotalEquity(bars) << std::endl;

                portfolio.executeOrder(order, true);

                std::cout << "INFO | Total Equity After: "
                        << std::setprecision(7)
                        << portfolio.getTotalEquity(bars) << std::endl;

                        std::cout << "DEBUG: equity: " << portfolio.getTotalEquity(bars) << std::endl;

                auto it = portfolio.getCurrentPositions().find("NQ");
                std::cout << "INFO | Total Positions After: " 
                        << (it != portfolio.getCurrentPositions().end() ? it->second.quantity : 0) 
                        << std::endl;

                std::cout << "----------------------------------------------"
                        << std::endl;
            }
        }
        //std::cout << "DEBUG: Logged time: " << bars.begin()->second.time << std::endl;
        //std::cout << "DEBUG: Logged Equity: " << portfolio.getTotalEquity(bars) << std::endl;

        // Record equity every bar (CRITICAL)
        equityCurve.push_back({
            bars.begin()->second.time,
            portfolio.getTotalEquity(bars)
        });

        ++barCount;
    }
    std::cout << "Finished loop" << std::endl;
    // -------------------------------------------------
    // Final liquidation
    // -------------------------------------------------
    std::map<std::string, Bar> finalBars = dataHandler.getCurrentBars();
    portfolio.closeAllPositions(finalBars);

    equityCurve.push_back({
        finalBars.begin()->second.time,
        portfolio.getTotalEquity(finalBars)
    });

    // -------------------------------------------------
    
    // Backtest summary
    // -------------------------------------------------
    std::cout << "\n=== Backtest Complete ===" << std::endl;
    std::cout << "Bars processed : " << barCount << std::endl;
    std::cout << "Trades         : " << portfolio.getAllTrades().size() << std::endl;
    std::cout << "Realized PnL   : " << portfolio.getRealizedPnL() << std::endl;
    std::cout << "Final Equity   : "
              << portfolio.getTotalEquity(finalBars) << std::endl;

    // -------------------------------------------------
    // Performance statistics
    // -------------------------------------------------
    std::cout << "\n=== Performance Statistics ===" << std::endl;

    double annReturn = Performance::annualizedReturn(
        equityCurve, Frequency::MINUTE);

    double annVol = Performance::annualizedVolatility(
        equityCurve, Frequency::MINUTE);

    double sharpe = Performance::sharpeRatio(
        equityCurve, Frequency::MINUTE, 0.0);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Annualized Return : " << annReturn * 100 << " %" << std::endl;
    std::cout << "Annualized Vol    : " << annVol * 100 << " %" << std::endl;
    std::cout << "Sharpe Ratio      : " << sharpe << std::endl;

    return 0;
}
