#pragma once

#include <string>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "backtest-cpp/portfolio.h"

struct StrategyConfig {
    std::string strategyType;
    YAML::Node strategyParameters;
    bool multiAsset;
};

struct TradingConfig { // move this to PortfolioConfig also later when used
    bool roundtripCommission;
    double spread;
};

struct LoggingConfig {
    bool debugLogs;
    bool systemPerfLogs;
};

class Config {
    public:
        StrategyConfig strategy_;
        PortfolioConfig portfolio_;
        TradingConfig trading_;
        LoggingConfig logging_;
        std::filesystem::path dataPath_;

        Config(const std::filesystem::path& configPath);
};
