#pragma once

#include<memory>
#include "backtest-cpp/config.h"
#include "backtest-cpp/singleAssetStrategy.h"

std::unique_ptr<SingleAssetStrategy> makeStrategy(const StrategyConfig& config, uint32_t symbolId);