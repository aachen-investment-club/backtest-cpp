#include "event.h"

#include "types.h"

// Base Event
Event::Event(time_t timestamp) : timestamp_(timestamp) {}

time_t Event::getTimestamp() const {
    return timestamp_;
}

// Event Queue
void EventQueue::push(const std::shared_ptr<Event> event) {
    queue_.push(event);
}

void EventQueue::pop() {
    queue_.pop();
}

std::shared_ptr<Event> EventQueue::front() const {
    return queue_.front();
}

size_t EventQueue::size() const {
    return queue_.size();
}

bool EventQueue::empty() const {
    return queue_.empty();
}

// MarketDataEvent
MarketDataEvent::MarketDataEvent(const Bar& bar) : Event(bar.time), bar_(bar) {}

EventType MarketDataEvent::getType() const {
    return EventType::MARKET;
}

const Bar& MarketDataEvent::getBar() const {
    return bar_;
}

// SignalEvent
SignalEvent::SignalEvent(const Signal& signal) : Event(signal.time), signal_(signal) {}

EventType SignalEvent::getType() const {
    return EventType::SIGNAL;
}

const Signal& SignalEvent::getSignal() const {
    return signal_;
}

// OrderEvent
OrderEvent::OrderEvent(const Order& order) : Event(order.time), order_(order) {}

EventType OrderEvent::getType() const {
    return EventType::ORDER;
}

const Order& OrderEvent::getOrder() const {
    return order_;
}

// Fill event
FillEvent::FillEvent(const Order& order) : Event(order.time), order_(order) {}

EventType FillEvent::getType() const {
    return EventType::FILL;
}

const Order& FillEvent::getFilledOrder() const {
    return order_;
}