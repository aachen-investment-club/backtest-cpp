#pragma once

#include "backtest-cpp/strategy.h"

class SMACrossover : public Strategy {
   public:
    SMACrossover(uint32_t sym_id, int shortPeriod = 10, int longPeriod = 30);

    void onInit(const std::vector<std::vector<Bar>>& availableData) override;

    std::unordered_map<uint32_t, std::optional<Signal>> onBars(
        std::vector<Bar>& bars, std::unordered_map<uint32_t, Position>& positions) override;
    Order generateOrder(const Signal& signal, const Bar& currentBar, const double& maxInvest,
                        std::unordered_map<uint32_t, Position>& positions) override;

    std::unordered_map<uint32_t, Order> generateOrders(const std::unordered_map<uint32_t, Signal>& signals,
                                             const std::vector<Bar>& currentBars,
                                             const double& maxInvest,
                                             std::unordered_map<uint32_t, Position>& positions) override;

   private:
    uint32_t symbol_id;
    int shortPeriod_;
    int longPeriod_;

    std::deque<double> shortWindow_;
    std::deque<double> longWindow_;

    double shortMA_ = 0.0;
    double longMA_ = 0.0;
    double prevShortMA_ = 0.0;  // Track previous for crossover detection
    double prevLongMA_ = 0.0;

    bool initialized_ = false;
};
