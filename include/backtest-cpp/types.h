#pragma once
#include <cstdint>
#include <ctime>
#include <string>
#include <cmath>

constexpr int64_t PRICE_PRECISION = 10000; // 10^n means n decimal digits for precision
inline int64_t priceDoubleToInt(double price) { 
    return std::llround(price * PRICE_PRECISION);
}
inline double priceIntToDouble(int64_t price) {
    return static_cast<double>(price) / PRICE_PRECISION;
}

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
    int64_t open;
    int64_t high;
    int64_t low;
    int64_t close;
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
    int64_t price;
    OrderType type;
    int quantity;
};

struct Trade {
    Order order;
    int quantity;
    int64_t pnl;
    int64_t commission;
};

struct Position {
    uint32_t symbol_id;
    int quantity;
    int64_t averagePrice;
};

struct PerformanceRunResult {
    double throughput = 0.0;
    double l1_miss_pct = 0.0;
    double branch_miss_pct = 0.0;
    double ipc = 0.0;
    double max_rss = 0.0;
};