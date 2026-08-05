#pragma once

#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <optional>
#include <vector>
#include <span>
#include <concepts>

#include "backtest-cpp/types.h"

// C++20 concept to ensure only valid strategies get accepted by templated runbacktest()
template <typename S>
concept SingleAssetStrategy = requires(S s,
                                    const Position position,
                                    const Bar bar) {
    { s.onInit() } -> std::same_as<void>; // Set up data, load config
    { s.onBar(bar) } -> std::same_as<std::optional<Order>>;
};

template <typename S>
concept MultiAssetStrategy = requires(S s, 
                                    std::span<const Position> positions,
                                    std::span<const Bar> bars) {
    { s.onInit() } -> std::same_as<void>;
    { s.onBars(bars) } -> std::same_as<std::span<std::optional<Order>>>;
};

template <typename S>
concept Strategy = SingleAssetStrategy<S> || MultiAssetStrategy<S>;

// old pure virutal class
// class Strategy {
//    public:
//     virtual ~Strategy() = default;
//     virtual void onInit(const std::vector<std::vector<Bar>>& availableData) = 0;

//     virtual std::map<uint32_t, std::optional<Signal>> onBars(
//         std::vector<Bar>& bars, std::map<uint32_t, Position>& positions) = 0;

//     virtual Order generateOrder(const Signal& signal, const Bar& currentBar,
//                                 const double& maxInvest,
//                                 std::map<uint32_t, Position>& positions) = 0;

//     virtual std::map<uint32_t, Order> generateOrders(const std::map<uint32_t, Signal>& signals,
//                                                      const std::vector<Bar>& currentBars,
//                                                      const double& maxInvest,
//                                                      std::map<uint32_t, Position>& positions) = 0;
// };
