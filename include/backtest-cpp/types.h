#pragma once
#include <cstdint>
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
    uint32_t symbol_id;
    int64_t time;
    double open;
    double high;
    double low;
    double close;
    long volume;
};

struct Signal {
    int64_t time;
    uint32_t symbol_id;
    SignalType type;
};

struct Order {
    int64_t time;
    uint32_t symbol_id;
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
    uint32_t symbol_id;
    int quantity;
    double averagePrice;
};
