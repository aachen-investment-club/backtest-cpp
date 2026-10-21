#pragma once

#include<memory>
#include "backtest-cpp/config.h"
#include "backtest-cpp/strategy.h"

std::unique_ptr<Strategy> makeStrategy(const StrategyConfig& config, uint32_t symbolId);