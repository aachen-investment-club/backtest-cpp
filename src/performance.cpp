#include "backtest-cpp/performance.h"

#include <cmath>
#include <stdexcept>
#include "backtest-cpp/types.h"

Annualization Performance::getAnnualization(Frequency freq) {  // TODO: make instrument specific
    switch (freq) {
        case Frequency::DAILY:
            return {252.0};
        case Frequency::HOURLY:
            return {252.0 * 23.0};  // Futures trading hours for our NQ example
        case Frequency::MINUTE:
            return {252.0 * 23.0 * 60.0};
        default:
            throw std::runtime_error("Unknown frequency");
    }
}

double Performance::annualizedReturn(const std::vector<EquityPoint>& curve, Frequency freq) {
    if (curve.size() < 2) throw std::runtime_error("Equity curve too short");

    double start =  priceIntToDouble(curve.front().equity);
    double end = priceIntToDouble(curve.back().equity);

    double periods = static_cast<double>(curve.size() - 1);
    double annualPeriods = getAnnualization(freq).periodsPerYear;

    return std::pow(end / start, annualPeriods / periods) - 1.0;
}

double Performance::annualizedVolatility(const std::vector<EquityPoint>& curve, Frequency freq) {
    if (curve.size() < 3) throw std::runtime_error("Equity curve too short");

    const size_t n = curve.size() - 1;
    double sum = 0.0, sumSq = 0.0;

    for (size_t i = 1; i < curve.size(); ++i) {
        double r = std::log(priceIntToDouble(curve[i].equity) /
                            priceIntToDouble(curve[i - 1].equity));
        sum += r;
        sumSq += r * r;
    }

    double mean = sum / static_cast<double>(n);
    double var = (sumSq - static_cast<double>(n) * mean * mean) /
                 static_cast<double>(n - 1);

    return std::sqrt(var * getAnnualization(freq).periodsPerYear);
}

double Performance::sharpeRatio(const std::vector<EquityPoint>& curve, Frequency freq,
                                double riskFreeRate) {
    double annReturn = annualizedReturn(curve, freq);
    double annVol = annualizedVolatility(curve, freq);

    if (annVol == 0.0) return 0.0;

    return (annReturn - riskFreeRate) / annVol;
}
