#include "backtest-cpp/strategies/strategy_factory.h"
#include "backtest-cpp/strategies/smacrossover.h"
#include "backtest-cpp/config.h"
#include "backtest-cpp/singleAssetStrategy.h"

#include <memory>
#include <iostream>

std::unique_ptr<SingleAssetStrategy> makeStrategy(const StrategyConfig& config, uint32_t symbolId) {
    if(config.strategyType == "SMACrossover") {
        return std::make_unique<SMACrossover>(
                                SMACrossover(symbolId,
                                config.strategyParameters["short_period"].as<int>(),
                                config.strategyParameters["long_period"].as<int>()));
    }
    else {
        throw std::runtime_error("Unknown strategy: " + config.strategyType);
    }
}
