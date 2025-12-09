#pragma once

#include <deque>
#include <map>
#include <optional>
#include <vector>

#include "types.h"

class Strategy {
   public:
    virtual ~Strategy() = default;
    virtual void onInit(const std::vector<Bar>& availableData) = 0;
    virtual std::optional<Signal> onBar(const Bar& bar,
                                        std::map<std::string, Position>& positions) = 0;
    virtual Order generateOrder(const Signal& signal, const Bar& currentBar,
                                const double& maxInvest,
                                std::map<std::string, Position>& positions) = 0;
};

class SMACrossover : public Strategy {
   public:
    SMACrossover(int shortPeriod = 10, int longPeriod = 30);
    void onInit(const std::vector<Bar>& availableData) override;
    std::optional<Signal> onBar(const Bar& bar,
                                std::map<std::string, Position>& positions) override;
    Order generateOrder(const Signal& signal, const Bar& currentBar, const double& maxInvest,
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