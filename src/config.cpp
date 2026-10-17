#include <yaml-cpp/yaml.h>
#include <string>
#include <memory>

#include "backtest-cpp/config.h"

Config::Config(const std::filesystem::path& configPath) {
    const YAML::Node root = YAML::LoadFile(configPath);
    
    const auto& strategy = root["strategy"]; 
    strategy_.strategyType = strategy["type"].as<std::string>();
    strategy_.multiAsset = strategy["multi_asset"].as<bool>();
    strategy_.strategyParameters = strategy["parameters"];

    const auto& trading = root["trading"];
    trading_.roundtripCommission = trading["roundtrip_commission"].as<bool>();
    portfolio_.commission = trading["commission"].as<double>();
    trading_.spread = trading["spread"].as<double>();

    const auto& logging = root["logging"];
    logging_.debugLogs = logging["debug_logs"].as<bool>();
    logging_.systemPerfLogs = logging["system_performance_logs"].as<bool>();

    const auto& portfolio = root["portfolio"];
    portfolio_.initialCash = portfolio["initial_cash"].as<double>();
    portfolio_.leverage = portfolio["leverage"].as<double>();

    dataPath_ = std::filesystem::path("data") / root["data"]["data_path"].as<std::string>();
}
