#pragma once

#include <cstdint>
#include <ctime>
#include <vector>

struct EquityPoint {
    int64_t time;
    double equity;
};

enum class Frequency { DAILY, MINUTE, HOURLY };

struct Annualization {
    double periodsPerYear;
};

class Performance {
   public:
    static double annualizedReturn(const std::vector<EquityPoint>& curve, Frequency freq);

    static double annualizedVolatility(const std::vector<EquityPoint>& curve, Frequency freq);

    static double sharpeRatio(const std::vector<EquityPoint>& curve, Frequency freq,
                              double riskFreeRate = 0.0);

   private:
    static Annualization getAnnualization(Frequency freq);
};
