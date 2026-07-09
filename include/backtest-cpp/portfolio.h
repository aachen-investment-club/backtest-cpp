#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "backtest-cpp/types.h"

struct PortfolioConfig {
    double initialCash;
    double commission;
    double leverage = 1.0;
};

class Portfolio {
   public:
    Portfolio(const PortfolioConfig& config);

    std::map<std::uint32_t, Position>& getCurrentPositions();
    int64_t getInvestedValue(const std::vector<Bar>& currentBars) const;
    int64_t getTotalEquity(const std::vector<Bar>& currentBar) const;
    int64_t getRealizedPnL() const;
    int64_t getUnrealizedPnL(const std::vector<Bar>& currentBars) const;
    bool checkOverdraft(const Order& order) const;
    std::vector<Order> getAllOrders(int64_t fromTime) const;
    std::vector<Trade> getAllTrades() const;
    int64_t getAvailableCash() const;
    void closeAllPositions(const std::vector<Bar>& currentBars);
    void executeOrder(const Order& order, const bool close = false);

   private:
    int64_t availableCash_ = 10000;
    const double leverage_ = 1;
    const int64_t commission_ = priceDoubleToInt(2.70);

    std::map<std::uint32_t, Position> positions_;  // Open Positions
    std::vector<Order> orders_;                    // Open Orders
    std::vector<Trade> trades_;                    // Elapsed Trades
};
