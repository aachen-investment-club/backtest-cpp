#include "backtest-cpp/strategies/smacrossover.h"

#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "backtest-cpp/types.h"


SMACrossover::SMACrossover(uint32_t sym_id, int shortPeriod, int longPeriod)
    : symbol_id(sym_id), shortPeriod_(shortPeriod), longPeriod_(longPeriod) {
    if (shortPeriod >= longPeriod) {
        throw std::invalid_argument("Short period must be < long period");
    }
}

// onInit(const std::map<std::string, std::vector<Bar>>& availableData)
void SMACrossover::onInit(const std::vector<Bar>& availableData) {
    size_t n = availableData.size();

    if (n < static_cast<size_t>(longPeriod_)) {
        throw std::runtime_error("Not enough historical data");
    }

    double shortSum = 0.0;
    double longSum = 0.0;

    for (size_t i = n - static_cast<size_t>(longPeriod_); i < n; i++) {
        double closePrice = priceIntToDouble(availableData[i].close);

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

void SMACrossover::onBar(const Bar& bar, std::vector<Signal>& signals) {
    
    if (!initialized_) {
        return;  // Not ready yet
    }
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
        signals.push_back(Signal{.time=bar.time, .symbol_id=symbol_id, .type=SignalType::BUY});
    } else if (previouslyAbove && !currentlyAbove) {
        signals.push_back(Signal{.time=bar.time, .symbol_id=symbol_id, .type=SignalType::SELL});
    }

    }


std::unordered_map<uint32_t, Order> SMACrossover::generateOrders(
    const std::vector<Signal>& signals, const Bar& currentBar,
    std::unordered_map<std::uint32_t, Position>& currentPosition) {
    
    std::unordered_map<uint32_t, Order> orderMap;

    for (const auto& [time, sig_symbol_id, signal_type] : signals) {
        // Get current position (can be positive, negative, or zero)
        int current_positionSize = 0;
        if (auto it = currentPosition.find(sig_symbol_id); it != currentPosition.end()) {
            current_positionSize = it->second.quantity;
        }

        // Calculate target position size
        int target_size = static_cast<int>(
            std::floor(maxInvest_ / priceIntToDouble(currentBar.open)));

        int quantity = 0;

        if (signal_type == SignalType::BUY) {
            // Target: LONG target_size
            quantity = target_size - current_positionSize;

        } else if (signal_type == SignalType::SELL) {
            // Target: SHORT target_size
            quantity = -target_size - current_positionSize;
        }
        orderMap[sig_symbol_id] = Order{.time=time,       .symbol_id=sig_symbol_id,
            .direction=signal_type,       .price=currentBar.close,
            .type=OrderType::MARKET, .quantity=quantity};
        
    }

    return orderMap;
}
