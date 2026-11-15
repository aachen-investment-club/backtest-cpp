#include "backtest-cpp/strategies/strategy_factory.h"
#include "backtest-cpp/config.h"
#include "backtest-cpp/strategy.h"

#include <memory>
#include <iostream>
#include <functional>
#include <concepts>

namespace {
    std::unordered_map<std::string, Creator>& getStrategyRegistry() { 
        // define registry as a static function variable to prevent static initialization order fiasco 
        static std::unordered_map<std::string, Creator> registry;
        return registry;
    }
}

void registerCreator(const std::string& strategyName, const Creator& creator) {
    getStrategyRegistry()[strategyName] = creator;
}

std::unique_ptr<Strategy> makeStrategy(const StrategyConfig& config, const uint32_t symbolId) {
    auto iter = getStrategyRegistry().find(config.strategyType);
    if(iter != getStrategyRegistry().end()) {
        return (iter->second)(config, symbolId);
    }
    else {
        throw std::runtime_error("Unknown strategy: " + config.strategyType);
    }
}
