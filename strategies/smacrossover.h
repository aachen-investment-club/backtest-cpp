#pragma once

#include "strategy.h"

class SMACrossover : public Strategy {
   public:
    SMACrossover(int shortPeriod = 10, int longPeriod = 30);

    void onInit(const std::vector<std::map<std::string, Bar>>& availableData) override;
    
    std::map<std::string, std::optional<Signal>> onBars(
         std::map<std::string, Bar>& bars,
         std::map<std::string, Position>& positions) override;
    Order generateOrder(const Signal& signal, const Bar& currentBar, 
         const double& maxInvest,
         std::map<std::string, Position>& positions) override;

    std::map<std::string, Order> generateOrders(
      const std::map<std::string, Signal>& signals, 
      const std::map<std::string, Bar>& currentBars,
      const double& maxInvest,
      std::map<std::string, Position>& positions) override;

   private:
    int shortPeriod_;
    int longPeriod_;

    std::deque<double> shortWindow_;
    std::deque<double> longWindow_;

    double shortMA_ = 0.0;
    double longMA_ = 0.0;
    double prevShortMA_ = 0.0;  // Track previous for crossover detection
    double prevLongMA_ = 0.0;

    bool initialized_ = false;
};