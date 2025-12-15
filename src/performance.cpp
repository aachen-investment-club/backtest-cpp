#include "performance.h"
#include <cmath>
#include <stdexcept>

Annualization Performance::getAnnualization(Frequency freq) { // TODO: make instrument specific
    switch (freq) {
        case Frequency::DAILY:
            return {252.0};
        case Frequency::HOURLY:
            return {252.0 * 23.0}; // Futures trading hours for our NQ example
        case Frequency::MINUTE:
            return {252.0 * 23.0 * 60.0};
        default:
            throw std::runtime_error("Unknown frequency");
    }
}

double Performance::annualizedReturn(
    const std::vector<EquityPoint>& curve,
    Frequency freq) {

    if (curve.size() < 2)
        throw std::runtime_error("Equity curve too short");

    double start = curve.front().equity;
    double end   = curve.back().equity;

    double periods = static_cast<double>(curve.size() - 1);
    double annualPeriods = getAnnualization(freq).periodsPerYear;

    return std::pow(end / start, annualPeriods / periods) - 1.0;
}


double Performance::annualizedVolatility(
    const std::vector<EquityPoint>& curve,
    Frequency freq) {

    if (curve.size() < 3)
        throw std::runtime_error("Equity curve too short");

    std::vector<double> returns;
    returns.reserve(curve.size() - 1);

    for (size_t i = 1; i < curve.size(); ++i) {
        returns.push_back(
            std::log(curve[i].equity / curve[i - 1].equity));
    }

    double mean = 0.0;
    for (double r : returns) mean += r;
    mean /= returns.size();

    double var = 0.0;
    for (double r : returns)
        var += (r - mean) * (r - mean);
    var /= (returns.size() - 1);

    double annualPeriods = getAnnualization(freq).periodsPerYear;
    return std::sqrt(var * annualPeriods);
}

double Performance::sharpeRatio(
    const std::vector<EquityPoint>& curve,
    Frequency freq,
    double riskFreeRate) {

    double annReturn = annualizedReturn(curve, freq);
    double annVol    = annualizedVolatility(curve, freq);

    if (annVol == 0.0)
        return 0.0;

    return (annReturn - riskFreeRate) / annVol;
}
