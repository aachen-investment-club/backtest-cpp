#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "strategy.h"
#include "types.h"

SMACrossover::SMACrossover(int shortPeriod, int longPeriod)
    : shortPeriod_(shortPeriod), longPeriod_(longPeriod) {
    if (shortPeriod >= longPeriod) {
        throw std::invalid_argument("Short period must be < long period");
    }
}

void SMACrossover::onInit(const std::vector<Bar>& availableData) {
    size_t n = availableData.size();

    if (n < static_cast<size_t>(longPeriod_)) {
        throw std::runtime_error("Not enough historical data");
    }

    double shortSum = 0.0;
    double longSum = 0.0;

    for (size_t i = n - longPeriod_; i < n; i++) {
        double closePrice = availableData[i].close;

        longWindow_.push_back(closePrice);
        longSum += closePrice;

        if (i >= n - shortPeriod_) {
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

std::optional<Signal> SMACrossover::onBar(const Bar& bar,
                                          std::map<std::string, Position>& positions) {
    if (!initialized_) {
        return std::nullopt;  // Not ready yet
    }

    double newPrice = bar.close;

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
        return Signal{bar.time, bar.symbol, SignalType::BUY};
    } else if (previouslyAbove && !currentlyAbove) {
        return Signal{bar.time, bar.symbol, SignalType::SELL};
    }

    // No crossover
    return std::nullopt;
}

Order SMACrossover::generateOrder(const Signal& signal, const Bar& currentBar,
                                  const double& maxInvest,
                                  std::map<std::string, Position>& positions) {
    // Get current position (can be positive, negative, or zero)
    auto it = positions.find(currentBar.symbol);
    int current_position = (it != positions.end()) ? it->second.quantity : 0;

    // Calculate target position size
    int target_size = static_cast<int>(std::floor(maxInvest / currentBar.open));

    int quantity = 0;

    if (signal.type == SignalType::BUY) {
        // Target: LONG target_size
        quantity = target_size - current_position;

    } else if (signal.type == SignalType::SELL) {
        // Target: SHORT target_size
        quantity = -target_size - current_position;
    }

    return Order{signal.time,      signal.symbol,     signal.type,
                 currentBar.close, OrderType::MARKET, quantity};
}