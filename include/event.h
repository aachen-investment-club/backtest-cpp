#pragma once

#include <memory>
#include <queue>
#include <vector>

#include "types.h"

// Base Event
class Event {
   public:
    virtual ~Event() = default;  // Virtual destructor!
    virtual EventType getType() const = 0;
    time_t getTimestamp() const;

   protected:
    Event(time_t timestamp);

   private:
    time_t timestamp_;
};

// Event queue
class EventQueue {
   public:
    EventQueue() = default;
    void push(const std::shared_ptr<Event> event);
    void pop();
    std::shared_ptr<Event> front() const;
    size_t size() const;
    bool empty() const;

   private:
    std::queue<std::shared_ptr<Event>> queue_;
};

// Event Types
class MarketDataEvent : public Event {
   public:
    MarketDataEvent(const Bar& bar);     // Constructor - takes a Bar and stores it
    EventType getType() const override;  // Override pure virtual from Event
    const Bar& getBar() const;           // Getter to access the bar data

   private:
    Bar bar_;  // Store the bar data
};

class SignalEvent : public Event {
   public:
    SignalEvent(const Signal& signal);
    EventType getType() const override;
    const Signal& getSignal() const;

   private:
    Signal signal_;
};

class OrderEvent : public Event {
   public:
    OrderEvent(const Order& order);
    EventType getType() const override;
    const Order& getOrder() const;

   private:
    Order order_;
};

class FillEvent : public Event {
   public:
    FillEvent(const Order& order);
    EventType getType() const override;
    const Order& getFilledOrder() const;

   private:
    Order order_;  // Filled order
};