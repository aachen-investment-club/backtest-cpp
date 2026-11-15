#pragma once

#include <string>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <memory>
#include "backtest-cpp/portfolio.h"
#include "backtest-cpp/strategy.h"

struct StrategyConfig {
    std::string strategyType;
    YAML::Node strategyParameters;
    bool multiAsset;
};

struct LoggingConfig {
    bool debugLogs;
    bool systemPerfLogs;
};

class Config {
    public:
        StrategyConfig strategy_;
        PortfolioConfig portfolio_;
        LoggingConfig logging_;
        std::string dataPath_;

        Config(const std::string& configPath);
};
