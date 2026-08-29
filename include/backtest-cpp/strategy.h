#pragma once

#include <cstdint>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <optional>
#include <vector>

#include "backtest-cpp/types.h"

class Strategy {
   public:
    virtual ~Strategy() = default;
    virtual void onInit(const std::vector<std::vector<Bar>>& availableData) = 0;

    virtual std::unordered_map<uint32_t, std::optional<Signal>> onBars(
        std::vector<Bar>& bars, std::unordered_map<uint32_t, Position>& positions) = 0;

    virtual Order generateOrder(const Signal& signal, const Bar& currentBar,
                                const double& maxInvest,
                                std::unordered_map<uint32_t, Position>& positions) = 0;

    virtual std::unordered_map<uint32_t, Order> generateOrders(const std::unordered_map<uint32_t, Signal>& signals,
                                                     const std::vector<Bar>& currentBars,
                                                     const double& maxInvest,
                                                     std::unordered_map<uint32_t, Position>& positions) = 0;
};