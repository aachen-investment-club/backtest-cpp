#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>

#include "data.h"
#include "performance.h"
#include "portfolio.h"
#include "strategy.h"

int main() {
    std::cout << "=== Backtesting Engine ===" << std::endl;

    // -------------------------------------------------
    // Initialization
    // -------------------------------------------------
    DataHandler dataHandler;
    Portfolio portfolio({.initialCash = 100'000.0,
                         .commission = 2.7,
                         .leverage = 1.0});

    SMACrossover strategy(10, 30);

    dataHandler.loadCSV("../data/Mini.csv");

    // -------------------------------------------------
    // Strategy warm-up (SMA lookback)
    // -------------------------------------------------
    std::vector<Bar> historicalData;
    for (int i = 0; i < 30 && dataHandler.hasMoreData(); ++i) {
        historicalData.push_back(dataHandler.getNextBar());
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
        Bar bar = dataHandler.getNextBar();

        std::optional<Signal> signal =
            strategy.onBar(bar, portfolio.getCurrentPositions());

        if (signal.has_value()) {
            Order order = strategy.generateOrder(
                signal.value(),
                bar,
                100000,
                portfolio.getCurrentPositions());

            std::cout << "Order at bar " << barCount << ": "
                      << (signal->type == SignalType::BUY ? "BUY " : "SELL ")
                      << order.quantity << " @ " << bar.close << std::endl;

            std::cout << "INFO | Unrealized PnL : "
                      << portfolio.getUnrealizedPnL(bar)
                      << " | Realized PnL : "
                      << portfolio.getRealizedPnL() << std::endl;

            std::cout << "INFO | Total Equity Before: "
                      << portfolio.getTotalEquity(bar) << std::endl;

            portfolio.executeOrder(order);

            std::cout << "INFO | Total Equity After: "
                      << std::setprecision(7)
                      << portfolio.getTotalEquity(bar) << std::endl;

            std::cout << "----------------------------------------------"
                      << std::endl;
        }

        // Record equity every bar (CRITICAL)
        equityCurve.push_back({
            bar.time,
            portfolio.getTotalEquity(bar)
        });

        ++barCount;
    }

    // -------------------------------------------------
    // Final liquidation
    // -------------------------------------------------
    Bar finalBar = dataHandler.getCurrentBar();
    portfolio.closeAllPositions(finalBar);

    equityCurve.push_back({
        finalBar.time,
        portfolio.getTotalEquity(finalBar)
    });

    // -------------------------------------------------
    // Backtest summary
    // -------------------------------------------------
    std::cout << "\n=== Backtest Complete ===" << std::endl;
    std::cout << "Bars processed : " << barCount << std::endl;
    std::cout << "Trades         : " << portfolio.getAllTrades().size() << std::endl;
    std::cout << "Realized PnL   : " << portfolio.getRealizedPnL() << std::endl;
    std::cout << "Final Equity   : "
              << portfolio.getTotalEquity(finalBar) << std::endl;

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
