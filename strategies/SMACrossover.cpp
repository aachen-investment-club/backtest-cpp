#include "backtest-cpp/strategies/smacrossover.h"

#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "backtest-cpp/types.h"
SMACrossover::SMACrossover(uint32_t sym_id, int shortPeriod, int longPeriod)
    : symbol_id(sym_id), shortPeriod_(shortPeriod), longPeriod_(longPeriod) {
    if (shortPeriod >= longPeriod) {
        throw std::invalid_argument("Short period must be < long period");
    }
}

// onInit(const std::map<std::string, std::vector<Bar>>& availableData)
void SMACrossover::onInit(const std::vector<std::vector<Bar>>& availableData) {
    size_t n = availableData.size();

    if (n < static_cast<size_t>(longPeriod_)) {
        throw std::runtime_error("Not enough historical data");
    }

    double shortSum = 0.0;
    double longSum = 0.0;

    for (size_t i = n - static_cast<size_t>(longPeriod_); i < n; i++) {
        double closePrice = priceIntToDouble(availableData[i].at(symbol_id).close);

        longWindow_.push_back(closePrice);
        longSum += closePrice;

        if (i >= n - static_cast<size_t>(shortPeriod_)) {
            shortWindow_.push_back(closePrice);
            shortSum += closePrice;
        }
    }

    shortMA_ = shortSum / shortPeriod_;
    longMA_ = longSum / longPeriod_;
    prevShortMA_ = shortMA_;
    prevLongMA_ = longMA_;

    initialized_ = true;
}

std::map<uint32_t, std::optional<Signal>> SMACrossover::onBars(std::vector<Bar>& bars,
                                                               std::map<uint32_t, Position>&) {
    if (!initialized_) {
        return {};  // Not ready yet
    }

    std::map<uint32_t, std::optional<Signal>> signalMap;

    for (const auto& bar : bars) {
        if (bar.symbol_id != this->symbol_id) continue;
        double newPrice = priceIntToDouble(bar.close);

        // Indicator update Logic
        prevShortMA_ = shortMA_;
        prevLongMA_ = longMA_;

        if (shortWindow_.size() >= static_cast<size_t>(shortPeriod_)) {
            shortMA_ -= shortWindow_.front() / shortPeriod_;
            shortWindow_.pop_front();
        }
        shortWindow_.push_back(newPrice);
        shortMA_ += newPrice / shortPeriod_;

        if (longWindow_.size() >= static_cast<size_t>(longPeriod_)) {
            longMA_ -= longWindow_.front() / longPeriod_;
            longWindow_.pop_front();
        }
        longWindow_.push_back(newPrice);
        longMA_ += newPrice / longPeriod_;

        // Trading Logic
        bool previouslyAbove = prevShortMA_ > prevLongMA_;
        bool currentlyAbove = shortMA_ > longMA_;

        if (!previouslyAbove && currentlyAbove) {
            signalMap[this->symbol_id] =
                Signal{bars[this->symbol_id].time, this->symbol_id, SignalType::BUY};
        } else if (previouslyAbove && !currentlyAbove) {
            signalMap[this->symbol_id] =
                Signal{bars[this->symbol_id].time, this->symbol_id, SignalType::SELL};
        }
    }

    return signalMap;
}

Order SMACrossover::generateOrder(const Signal& signal, const Bar& currentBar,
                                  const double& maxInvest,
                                  std::map<uint32_t, Position>& positions) {
    // Get current position (can be positive, negative, or zero)
    auto it = positions.find(currentBar.symbol_id);
    int current_position = (it != positions.end()) ? it->second.quantity : 0;

    // Calculate target position size
    int target_size = static_cast<int>(std::floor(maxInvest / priceIntToDouble(currentBar.open)));

    int quantity = 0;

    if (signal.type == SignalType::BUY) {
        // Target: LONG target_size
        quantity = target_size - current_position;

    } else if (signal.type == SignalType::SELL) {
        // Target: SHORT target_size
        quantity = -target_size - current_position;
    }

    return Order{signal.time,      signal.symbol_id,  signal.type,
                 currentBar.close, OrderType::MARKET, quantity};
}

std::map<uint32_t, Order> SMACrossover::generateOrders(const std::map<uint32_t, Signal>& signals,
                                                       const std::vector<Bar>& currentBars,
                                                       const double& maxInvest,
                                                       std::map<uint32_t, Position>& positions) {
    std::map<uint32_t, Order> orderMap;

    for (const auto& [sig_symbol_id, signal] : signals) {
        // Get current position (can be positive, negative, or zero)
        int current_positionSize = 0;
        if (auto it = positions.find(sig_symbol_id); it != positions.end()) {
            current_positionSize = it->second.quantity;
        }

        // Calculate target position size
        int target_size =
            static_cast<int>(std::floor(maxInvest / priceIntToDouble(currentBars.at(sig_symbol_id).open)));

        int quantity = 0;

        if (signal.type == SignalType::BUY) {
            // Target: LONG target_size
            quantity = target_size - current_positionSize;

        } else if (signal.type == SignalType::SELL) {
            // Target: SHORT target_size
            quantity = -target_size - current_positionSize;
        }

        orderMap[sig_symbol_id] = Order{signal.time,       signal.symbol_id,
                                        signal.type,       currentBars.at(sig_symbol_id).close,
                                        OrderType::MARKET, quantity};
    }

    return orderMap;
}
