#include <iomanip>
#include <iostream>

#include "data.h"
#include "portfolio.h"
#include "strategy.h"

int main() {
    std::cout << "=== Backtesting Engine ===" << std::endl;

    DataHandler dataHandler;
    Portfolio portfolio({.initialCash = 100'000.0, .commission = 2.7, .leverage = 1.0});
    SMACrossover strategy(10, 30);

    dataHandler.loadCSV("../data/Mini.csv");

    // Initialize strategy with historical data
    std::vector<Bar> historicalData;
    for (int i = 0; i < 30 && dataHandler.hasMoreData(); i++) {
        historicalData.push_back(dataHandler.getNextBar());
    }
    strategy.onInit(historicalData);

    std::cout << "Starting backtest..." << std::endl;
    int barCount = 0;

    // Main backtest loop
    while (dataHandler.hasMoreData()) {
        Bar bar = dataHandler.getNextBar();

        std::optional<Signal> signal = strategy.onBar(bar, portfolio.getCurrentPositions());

        if (signal.has_value()) {
            Order order = strategy.generateOrder(signal.value(), bar, 100000,
                                                 portfolio.getCurrentPositions());
            std::cout << "Order at bar " << barCount << ": "
                      << (signal->type == SignalType::BUY ? "BUY " : "SELL ") << order.quantity
                      << " @ " << bar.close
                      << " | Unrealized PnL : " << portfolio.getUnrealizedPnL(bar)
                      << " | Realized PnL: : " << portfolio.getRealizedPnL() << std::endl;
            std::cout << "INFO | Invested Value Before: " << portfolio.getInvestedValue(bar)
                      << std::endl;
            std::cout << "INFO | Total Equity Before: " << portfolio.getTotalEquity(bar)
                      << std::endl;
            portfolio.executeOrder(order);
            std::cout << "INFO | Invested Value After: " << portfolio.getInvestedValue(bar)
                      << std::endl;
            std::cout << "INFO | Total Equity After: " << portfolio.getTotalEquity(bar)
                      << std::setprecision(7) << std::endl;

            std::cout << "----------------------------------------------" << std::endl;
        }

        barCount++;
    }
    portfolio.closeAllPositions(dataHandler.getCurrentBar());
    std::cout << "closeAllPositions ran" << std::endl;

    std::cout << "\n=== Backtest Complete ===" << std::endl;
    std::cout << "Bars processed: " << barCount << std::endl;
    std::cout << "Total PnL: " << portfolio.getRealizedPnL() << std::endl;
    std::cout << "Final Equity: " << portfolio.getTotalEquity(dataHandler.getCurrentBar())
              << std::endl;
    std::cout << "Trades: " << portfolio.getAllTrades().size() << std::endl;

    return 0;
}