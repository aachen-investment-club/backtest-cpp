#pragma once
#include <ctime>
#include <string>

enum class SignalType { BUY, SELL, HOLD };

enum class OrderType { MARKET, LIMIT, STOP };

enum class EventType {
    MARKET,  // New market data
    SIGNAL,  // Strategy signal
    ORDER,   // Order to execute
    FILL     // Order executed
};

struct Bar {
    std::string symbol;
    time_t time;
    double open;
    double high;
    double low;
    double close;
    long volume;
};

struct Signal {
    time_t time;
    std::string symbol;
    SignalType type;
};

struct Order {
    time_t time;
    std::string symbol;
    SignalType direction;
    double price;
    OrderType type;
    int quantity;
};

struct Trade {
    Order order;
    int quantity;
    double pnl;
    double commission;
};

struct Position {
    std::string symbol;
    int quantity;
    double averagePrice;
    SignalType direction;
};
