#pragma once

#include <deque>
#include <map>
#include <optional>
#include <vector>
#include <cstring>

#include "types.h"

class Strategy {
   public:
    virtual ~Strategy() = default;
    virtual void onInit(const std::vector<std::map<std::string, Bar>>& availableData) = 0;
    
    virtual std::map<std::string, std::optional<Signal>> onBars(
         std::map<std::string, Bar>& bars,
         std::map<std::string, Position>& positions) = 0;

    virtual Order generateOrder(const Signal& signal, const Bar& currentBar,
                                const double& maxInvest,
                                std::map<std::string, Position>& positions) = 0;
         
    virtual std::map<std::string, Order> generateOrders(
      const std::map<std::string, Signal>& signals, 
      const std::map<std::string, Bar>& currentBars,
      const double& maxInvest,
      std::map<std::string, Position>& positions) = 0;
};