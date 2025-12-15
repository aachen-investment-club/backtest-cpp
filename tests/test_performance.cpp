#include <gtest/gtest.h>
#include <vector>
#include <cmath>

#include "performance.h"

// Helper function to create a simple equity curve
std::vector<EquityPoint> makeCurve(const std::vector<double>& equities, time_t startTime = 0, int step = 60) {
    std::vector<EquityPoint> curve;
    time_t t = startTime;
    for (double e : equities) {
        curve.push_back({t, e});
        t += step;
    }
    return curve;
}

// -----------------------------
// Annualized Return Tests
// -----------------------------
TEST(PerformanceTest, AnnualizedReturnSimpleGrowth) {
    auto curve = makeCurve({100, 101, 102.01}); // +1% per step
    double annRet = Performance::annualizedReturn(curve, Frequency::DAILY);
    // Expected approx: (121/100)^(252/2) - 1
    EXPECT_NEAR(annRet, 11.247, 0.1);
}

TEST(PerformanceTest, AnnualizedReturnSingleBar) {
    auto curve = makeCurve({100});
    EXPECT_THROW(Performance::annualizedReturn(curve, Frequency::DAILY), std::runtime_error);
}

TEST(PerformanceTest, AnnualizedReturnNoGrowth) {
    auto curve = makeCurve({100, 100, 100});
    double annRet = Performance::annualizedReturn(curve, Frequency::DAILY);
    EXPECT_NEAR(annRet, 0.0, 1e-8);
}

// -----------------------------
// Annualized Volatility Tests
// -----------------------------
TEST(PerformanceTest, VolatilitySimple) {
    auto curve = makeCurve({100, 102, 101, 103});
    double vol = Performance::annualizedVolatility(curve, Frequency::DAILY);
    EXPECT_NEAR(vol, 0.271, 0.1);
}

TEST(PerformanceTest, VolatilityConstantCurve) {
    auto curve = makeCurve({100, 100, 100, 100});
    double vol = Performance::annualizedVolatility(curve, Frequency::DAILY);
    EXPECT_NEAR(vol, 0.0, 1e-8);
}

TEST(PerformanceTest, VolatilityTooShort) {
    auto curve = makeCurve({100, 101});
    EXPECT_THROW(Performance::annualizedVolatility(curve, Frequency::DAILY), std::runtime_error);
}

// -----------------------------
// Sharpe Ratio Tests
// -----------------------------
TEST(PerformanceTest, SharpePositive) {
    auto curve = makeCurve({100, 101, 100, 103});
    double sr = Performance::sharpeRatio(curve, Frequency::DAILY, 0.0);
    EXPECT_NEAR(sr, 35, 0.1);
}

TEST(PerformanceTest, SharpeZeroVolatility) {
    auto curve = makeCurve({100, 100, 100});
    double sr = Performance::sharpeRatio(curve, Frequency::DAILY, 0.0);
    EXPECT_EQ(sr, 0);
}

// -----------------------------
// Frequency Tests
// -----------------------------
TEST(PerformanceTest, FrequencyMinuteCurve) {
    auto curve = makeCurve({100, 101, 102, 103, 104});
    double annRet = Performance::annualizedReturn(curve, Frequency::MINUTE);
    double vol = Performance::annualizedVolatility(curve, Frequency::MINUTE);
    double sr = Performance::sharpeRatio(curve, Frequency::MINUTE);
    EXPECT_GT(annRet, 0);
    EXPECT_GT(vol, 0);
    EXPECT_GT(sr, 0);
}