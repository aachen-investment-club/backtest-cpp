#pragma once

#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "backtest-cpp/types.h"

class SingleAssetStrategy {
   public:
    virtual ~SingleAssetStrategy() = default;
    virtual void onInit(const std::vector<Bar>& availableData) = 0;

    virtual void onBar(const Bar& bar, std::vector<Signal>& signals) = 0;

    virtual std::unordered_map<uint32_t, Order> generateOrders(
        const std::vector<Signal>& signals, const Bar& currentBar,
        std::unordered_map<std::uint32_t, Position>& currentPosition) = 0;
};