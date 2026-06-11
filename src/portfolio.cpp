#include "backtest-cpp/portfolio.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <cstdlib> 

#include "backtest-cpp/types.h"

Portfolio::Portfolio(const PortfolioConfig& config)
    : availableCash_(config.initialCash),
      leverage_(config.leverage),
      commission_(config.commission) {}

std::map<std::uint32_t, Position>& Portfolio::getCurrentPositions() {
    return positions_;
};

double Portfolio::getInvestedValue(const std::map<std::uint32_t, Bar>& currentBars) const {
    double totalPositionValue = 0;
    for (const auto& [symbol_id, position] : positions_) {
        // std::cout << "CurrentBars.size() " << currentBars.size() << std::endl;
        auto it = currentBars.find(symbol_id);
        if (it == currentBars.end()) {
            // Use last known price or throw error - don't just skip!
            std::cerr << "ERROR: Missing price for position " << symbol_id << std::endl;
            // throw std::runtime_error("Cannot calculate equity without price");
        }
        totalPositionValue += position.quantity * it->second.close;
    }
    return fabs(totalPositionValue);
}

double Portfolio::getTotalEquity(const std::map<std::uint32_t, Bar>& currentBars) const {
    return getInvestedValue(currentBars) + availableCash_;
};

std::vector<Order> Portfolio::getAllOrders(int64_t fromTime) const {
    std::vector<Order> ordersWithinTimeline;
    for (const Order& order : orders_) {
        if (order.time >= fromTime) {
            ordersWithinTimeline.push_back(order);
        }
    }
    return ordersWithinTimeline;
};

void Portfolio::closeAllPositions(const std::map<std::uint32_t, Bar>& currentBars) {
    auto it = positions_.begin();
    while (it != positions_.end()) {
        const uint32_t symbol_id = it->first;
        const Position& position = it->second;

        // Check if bar exists
        auto barIt = currentBars.find(symbol_id);
        if (barIt == currentBars.end()) {
            std::cerr << "WARNING: No price data for symbol " << symbol_id << std::endl;
            //++it;
            // continue;
        }

        // Build order using const references (no copies)
        Order closeOrder{.time = barIt->second.time,
                         .symbol_id = symbol_id,
                         .direction = (position.quantity > 0) ? SignalType::SELL : SignalType::BUY,
                         .price = barIt->second.close,
                         .type = OrderType::MARKET,
                         .quantity = -position.quantity};

        // CRITICAL: Increment iterator BEFORE executeOrder modifies positions_
        ++it;

        executeOrder(closeOrder, true);
    }
}

bool Portfolio::checkOverdraft(const Order& order) const {
    auto posIt = positions_.find(order.symbol_id);
    bool hasPosition = (posIt != positions_.end());

    if (hasPosition) {
        const Position& pos = positions_.at(order.symbol_id);
        int netPositionSize = pos.quantity + order.quantity;

        return (abs(netPositionSize) * order.price + commission_ >
                (availableCash_ * leverage_ + pos.averagePrice * abs(pos.quantity) - commission_));
    } else {
        return (abs(order.quantity) * order.price + commission_) >
               (availableCash_ * leverage_);  // NEEDS fixing for adjusting pos size
    }
}

double Portfolio::getRealizedPnL() const {
    double totalPnl = 0;
    for (const auto& trade : trades_) {
        totalPnl += trade.pnl;
    }
    return totalPnl;
}

double Portfolio::getUnrealizedPnL(const std::map<std::uint32_t, Bar>& currentBars) const {
    double UnrealizedPnl = 0;
    for (const auto& [symbol_id, position] : positions_) {
        UnrealizedPnl +=
            position.quantity * (currentBars.at(symbol_id).close - position.averagePrice) -
            commission_;
    }
    return UnrealizedPnl;
}

void Portfolio::executeOrder(const Order& order, const bool close = false) {
    auto posIt = positions_.find(order.symbol_id);
    bool hasPosition = (posIt != positions_.end());

    if (order.quantity == 0) {
        std::cerr << "Order quantity cannot be 0" << std::endl;
        return;
    }

    if (!close && checkOverdraft(order)) {
        std::cerr << "Insufficient funds for order" << std::endl;
        return;
    }

    // NEW POSITION
    if (!hasPosition) {
        positions_[order.symbol_id] = Position{
            .symbol_id = order.symbol_id,
            .quantity = order.quantity,
            .averagePrice = order.price,
            .direction = (order.quantity > 0) ? SignalType::BUY : SignalType::SELL,
        };

        double totalCost = fabs(order.quantity) * order.price + commission_;
        availableCash_ -= totalCost;

        // Adjust position
    } else {
        Position& pos = positions_[order.symbol_id];

        // Add to position
        if ((order.direction == SignalType::BUY && pos.direction == SignalType::BUY) ||
            (order.direction == SignalType::SELL && pos.direction == SignalType::SELL)) {
            pos.averagePrice = (pos.quantity * pos.averagePrice + order.quantity * order.price) /
                               static_cast<double>(pos.quantity + order.quantity);
            availableCash_ -= (order.quantity * order.price + commission_);

            // Remove from position
        } else {
            int netPositionSize = pos.quantity + order.quantity;

            int closedQuantity =
                abs(order.quantity) >= abs(pos.quantity) ? pos.quantity : order.quantity;
            double tradePnl = closedQuantity * (order.price - pos.averagePrice) - commission_;
            trades_.push_back(Trade{.order = order,
                                    .quantity = closedQuantity,
                                    .pnl = tradePnl,
                                    .commission = commission_});
            // DEBUG    
            // std::cout << "Logged Trade | " << "Closed: " << closedQuantity << " Entered @ "
            //           << pos.averagePrice << " Exited @ " << order.price << " P&L: " << tradePnl
            //           << std::endl;

            availableCash_ += (abs(closedQuantity) * pos.averagePrice + tradePnl + commission_);

            if (abs(order.quantity) > abs(pos.quantity)) {
                availableCash_ -= (abs(pos.quantity + order.quantity) * order.price + commission_);
            }

            pos.quantity = netPositionSize;
            pos.averagePrice =
                abs(pos.quantity) > abs(order.quantity) ? pos.averagePrice : order.price;
            pos.direction =
                abs(pos.quantity) > abs(order.quantity) ? pos.direction : order.direction;

            // Remove Empty Position
            if (netPositionSize == 0) {
                positions_.erase(order.symbol_id);
            }
        }
    }

    orders_.push_back(order);
}

std::vector<Trade> Portfolio::getAllTrades() const {
    return trades_;
}

double Portfolio::getAvailableCash() const {
    return availableCash_;
}
