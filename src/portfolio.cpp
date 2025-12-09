#include "portfolio.h"

#include <cmath>
#include <iostream>
#include <vector>

#include "types.h"

Portfolio::Portfolio(const PortfolioConfig& config)
    : availableCash_(config.initialCash),
      commission_(config.commission),
      leverage_(config.leverage) {}

std::map<std::string, Position>& Portfolio::getCurrentPositions() {
    return positions_;
};

const double Portfolio::getInvestedValue(const Bar& currentBar) const {
    double currentPrice = currentBar.close;  // Single-instrument (for now)
    double totalPositionValue = 0;
    for (const auto& [_, position] : positions_) {
        totalPositionValue += position.quantity * currentPrice;
    }
    return fabs(totalPositionValue);
}

const double Portfolio::getTotalEquity(const Bar& currentBar) const {
    return getInvestedValue(currentBar) + availableCash_;
};

std::vector<Order> Portfolio::getAllOrders(time_t fromTime) const {
    std::vector<Order> ordersWithinTimeline;
    for (const Order& order : orders_) {
        if (order.time >= fromTime) {
            ordersWithinTimeline.push_back(order);
        }
    }
    return ordersWithinTimeline;
};

void Portfolio::closeAllPositions(const Bar& currentBar) {
    auto it = positions_.begin();

    while (it != positions_.end()) {
        const Position& pos = it->second;  // Reference, no copy!

        Order closeOrder{.time = currentBar.time,
                         .symbol = pos.symbol,
                         .direction = (pos.quantity > 0) ? SignalType::SELL : SignalType::BUY,
                         .price = currentBar.close,
                         .type = OrderType::MARKET,
                         .quantity = -pos.quantity};

        ++it;
        executeOrder(closeOrder);
    }
}

bool Portfolio::checkOverdraft(const Order& order) const {
    auto posIt = positions_.find(order.symbol);
    bool hasPosition = (posIt != positions_.end());

    if (hasPosition) {
        const Position& pos = positions_.at(order.symbol);
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

double Portfolio::getUnrealizedPnL(const Bar& currentBar) const {
    double currentPrice = currentBar.close;  // Single-instrument (for now)
    double UnrealizedPnl = 0;
    for (const auto& [_, position] : positions_) {
        UnrealizedPnl += position.quantity * (currentPrice - position.averagePrice) - commission_;
    }
    return UnrealizedPnl;
}

void Portfolio::executeOrder(const Order& order) {
    auto posIt = positions_.find(order.symbol);
    bool hasPosition = (posIt != positions_.end());

    if (order.quantity == 0) {
        std::cerr << "Order quantity cannot be 0" << std::endl;
        return;
    }

    if (checkOverdraft(order)) {
        std::cerr << "Insufficient funds for order" << std::endl;
        return;
    }

    // NEW POSITION
    if (!hasPosition) {
        positions_[order.symbol] = Position{
            .symbol = order.symbol,
            .quantity = order.quantity,
            .averagePrice = order.price,
            .direction = (order.quantity > 0) ? SignalType::BUY : SignalType::SELL,
        };

        double totalCost = fabs(order.quantity) * order.price + commission_;
        availableCash_ -= totalCost;

        // Adjust position
    } else {
        Position& pos = positions_[order.symbol];

        // Add to position
        if (order.direction == SignalType::BUY && pos.direction == SignalType::BUY ||
            order.direction == SignalType::SELL && pos.direction == SignalType::SELL) {
            pos.averagePrice = (pos.quantity * pos.averagePrice + order.quantity * order.price) /
                               (double)(pos.quantity + order.quantity);
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
            std::cout << "Logged Trade | "
                      << "Closed: " << closedQuantity << " Entered @ " << pos.averagePrice
                      << " Exited @ " << order.price
                      << " Price diff: " << order.price - pos.averagePrice
                      << " PnL (w commission): " << tradePnl << std::endl;

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
                positions_.erase(order.symbol);
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