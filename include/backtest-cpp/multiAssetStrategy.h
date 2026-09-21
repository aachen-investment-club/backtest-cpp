#pragma once

#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "backtest-cpp/types.h"

class MultiAssetStrategy {
   public:
    virtual ~MultiAssetStrategy() = default;
    virtual void onInit(const std::vector<std::vector<Bar>>& availableData) = 0;

    virtual void onBars(std::vector<Bar>& bars, std::unordered_map<uint32_t, Position>& positions,
                        std::vector<Signal>& signals) = 0;

    virtual std::vector<Order>& generateOrders(
        const std::vector<Signal>& signals, const std::vector<Bar>& currentBars,
        std::unordered_map<uint32_t, Position>& positions) = 0;
};