#pragma once

#include <map>
#include <optional>
#include <vector>

#include "types.h"

struct PortfolioConfig {
    double initialCash;
    double commission;
    double leverage = 1.0;
};

class Portfolio {
   public:
    Portfolio(const PortfolioConfig& config);

    std::map<std::string, Position>& getCurrentPositions();
    const double getInvestedValue(const std::map<std::string, Bar>& currentBars) const;
    const double getTotalEquity(const std::map<std::string, Bar>& currentBar) const;
    double getRealizedPnL() const;
    double getUnrealizedPnL(const std::map<std::string, Bar>& currentBars) const;
    bool checkOverdraft(const Order& order) const;
    std::vector<Order> getAllOrders(time_t fromTime) const;
    std::vector<Trade> getAllTrades() const;
    double getAvailableCash() const;
    void closeAllPositions(const std::map<std::string, Bar>& currentBars);
    void executeOrder(const Order& order, const bool close);


   private:
    double availableCash_ = 10000;
    const double leverage_ = 1;
    const double commission_ = 2.70;

    std::map<std::string, Position> positions_;  // Open Positions
    std::vector<Order> orders_;                  // Open Orders
    std::vector<Trade> trades_;                  // Elapsed Trades
};