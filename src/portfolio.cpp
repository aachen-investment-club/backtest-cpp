#include "backtest-cpp/portfolio.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "backtest-cpp/types.h"

Portfolio::Portfolio(const PortfolioConfig& config)
    : availableCash_(priceDoubleToInt(config.initialCash)),
      leverage_(config.leverage),
      commission_(priceDoubleToInt(config.commission)) {}

std::map<std::uint32_t, Position>& Portfolio::getCurrentPositions() {
    return positions_;
};

int64_t Portfolio::getInvestedValue(const std::vector<Bar>& currentBars) const {
    int64_t totalPositionValue = 0;
    for (const auto& [symbol_id, position] : positions_) {
        // std::cout << "CurrentBars.size() " << currentBars.size() << std::endl;
        if (currentBars.at(symbol_id).symbol_id ==
            0) {  // ids start from 1, so 0 means doesn't exist
            // Use last known price or throw error - don't just skip!
            std::cerr << "ERROR: Missing price for position " << symbol_id << std::endl;
            throw std::runtime_error(
                "Cannot calculate equity without price");  // TODO create a last available price
                                                           // vector
        }
        totalPositionValue +=
            std::abs(position.quantity) * position.averagePrice +
            position.quantity * (currentBars.at(symbol_id).close - position.averagePrice);
    }
    return totalPositionValue;
}

int64_t Portfolio::getTotalEquity(const std::vector<Bar>& currentBars) const {
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

void Portfolio::closeAllPositions(const std::vector<Bar>& currentBars) {
    auto it = positions_.begin();
    while (it != positions_.end()) {
        const uint32_t symbol_id = it->first;
        const Position& position = it->second;

        // Check if bar exists
        if (currentBars.at(symbol_id).symbol_id == 0) {
            std::cerr << "WARNING: No price data for symbol " << symbol_id << std::endl;
            ++it;
            continue;  // TODO create a last available price vector
        }

        // Build order using const references (no copies)
        Order closeOrder{.time = currentBars.at(symbol_id).time,
                         .symbol_id = symbol_id,
                         .direction = (position.quantity > 0) ? SignalType::SELL : SignalType::BUY,
                         .price = currentBars.at(symbol_id).close,
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
        // if(pos.quantity * order.quantity < 0 && std::abs(pos.quantity) <
        // std::abs(order.quantity)) { // checks if new order flips the position
        //     return (std::abs(netPositionSize) * order.price + commission_ >
        //         (availableCash_ * leverage_ + order.price * std::abs(pos.quantity) -
        //         commission_)); // account for double commision
        // }
        return (
            static_cast<double>(std::abs(netPositionSize) * order.price + commission_) >
            (static_cast<double>(availableCash_) * leverage_ +
             static_cast<double>(
                 order.price * std::abs(pos.quantity))));  // TODO create a last available price
                                                           // vector and use it instead order.price
    } else {
        return static_cast<double>(std::abs(order.quantity) * order.price + commission_) >
               (static_cast<double>(availableCash_) * leverage_);
    }
}

int64_t Portfolio::getRealizedPnL() const {
    int64_t totalPnl = 0;
    for (const auto& trade : trades_) {
        totalPnl += trade.pnl - trade.commission;
    }
    return totalPnl;
}

int64_t Portfolio::getUnrealizedPnL(const std::vector<Bar>& currentBars) const {
    int64_t UnrealizedPnl = 0;
    for (const auto& [symbol_id, position] : positions_) {
        UnrealizedPnl +=
            position.quantity * (currentBars.at(symbol_id).close - position.averagePrice) -
            commission_;
    }
    return UnrealizedPnl;
}

void Portfolio::executeOrder(const Order& order, const bool close) {
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

    if (!hasPosition) {  // OPEN
        positions_[order.symbol_id] = Position{
            .symbol_id = order.symbol_id, .quantity = order.quantity, .averagePrice = order.price};
        trades_.push_back(Trade{
            .order = order,
            .quantity = 0,
            .pnl = 0,
            .commission =
                commission_});  // add a trade to account for the commision while calculating Pnl
        int64_t totalCost = std::abs(order.quantity) * order.price + commission_;
        availableCash_ -= totalCost;
        // Adjust position
    } else {
        Position& pos = positions_[order.symbol_id];

        if ((order.quantity > 0) == (pos.quantity > 0)) {  // ADD
            pos.averagePrice = priceDoubleToInt(
                static_cast<double>((pos.quantity * pos.averagePrice +
                                     order.quantity * order.price)) /  // Avoid integer division
                (pos.quantity + order.quantity));
            availableCash_ -= (std::abs(order.quantity) * order.price + commission_);
            pos.quantity += order.quantity;
            trades_.push_back(Trade{.order = order,
                                    .quantity = 0,
                                    .pnl = 0,
                                    .commission = commission_});  // add a trade to account for the
                                                                  // commision while calculating Pnl
        } else if (std::abs(order.quantity) < std::abs(pos.quantity)) {  // REDUCE
            int closedQuantity = -order.quantity;
            int64_t tradePnl = closedQuantity * (order.price - pos.averagePrice);
            trades_.push_back(Trade{.order = order,
                                    .quantity = closedQuantity,
                                    .pnl = tradePnl,
                                    .commission = commission_});
            availableCash_ +=
                (std::abs(closedQuantity) * pos.averagePrice + tradePnl - commission_);
            // average price doesn't change
            pos.quantity += order.quantity;
        } else if (std::abs(order.quantity) > std::abs(pos.quantity)) {  // FLIP
            int closedQuantity = pos.quantity;
            int64_t tradePnl = closedQuantity * (order.price - pos.averagePrice);
            trades_.push_back(Trade{.order = order,
                                    .quantity = closedQuantity,
                                    .pnl = tradePnl,
                                    .commission = commission_});
            availableCash_ +=
                (std::abs(closedQuantity) * pos.averagePrice + tradePnl - commission_);
            availableCash_ -= (std::abs(pos.quantity + order.quantity) * order.price);
            pos.averagePrice = order.price;

            pos.quantity += order.quantity;

        } else {  // CLOSE
            int closedQuantity = pos.quantity;
            int64_t tradePnl = closedQuantity * (order.price - pos.averagePrice);
            trades_.push_back(Trade{.order = order,
                                    .quantity = closedQuantity,
                                    .pnl = tradePnl,
                                    .commission = commission_});
            availableCash_ +=
                (std::abs(closedQuantity) * pos.averagePrice + tradePnl - commission_);
            positions_.erase(order.symbol_id);
        }
    }

    orders_.push_back(order);
}

std::vector<Trade> Portfolio::getAllTrades() const {
    return trades_;
}

int64_t Portfolio::getAvailableCash() const {
    return availableCash_;
}
