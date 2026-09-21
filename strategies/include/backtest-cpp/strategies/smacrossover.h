#pragma once

#include "backtest-cpp/singleAssetStrategy.h"

class SMACrossover : public SingleAssetStrategy {
   public:
    SMACrossover(uint32_t sym_id, int shortPeriod = 10, int longPeriod = 30);

    void onInit(const std::vector<Bar>& availableData) override;

    void onBar(const Bar& bar, std::vector<Signal>& signals) override;

    std::unordered_map<uint32_t, Order> generateOrders(
        const std::vector<Signal>& signals, const Bar& currentBar,
        std::unordered_map<std::uint32_t, Position>& currentPosition) override;

   private:
    uint32_t symbol_id;
    int maxInvest_;
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
