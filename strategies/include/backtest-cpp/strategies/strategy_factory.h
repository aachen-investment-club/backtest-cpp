#pragma once

#include "backtest-cpp/config.h"
#include "backtest-cpp/strategy.h"

#include <memory>
#include <iostream>
#include <functional>
#include <concepts>

using Creator = std::function<std::unique_ptr<Strategy>(const StrategyConfig&, const uint32_t)>;

void registerCreator(const std::string& strategyName, const Creator& creator); // a template free function to keep the strategyRegistry only in .cpp file

template<typename T>
    requires std::derived_from<T, Strategy> && std::constructible_from<T, const StrategyConfig&, const uint32_t>
bool registerStrategy(const std::string& strategyName) {
    registerCreator(strategyName, [](const StrategyConfig& config, const uint32_t symbolId) {
        return std::make_unique<T>(config, symbolId);        
    });
    return 0;
}

#define REGISTER_STRATEGY(StrategyClass, StrategyName) \
    [[maybe_unused]] static const bool registered_##StrategyClass \
       = registerStrategy<StrategyClass>(StrategyName)


std::unique_ptr<Strategy> makeStrategy(const StrategyConfig& config, const uint32_t symbolId);
