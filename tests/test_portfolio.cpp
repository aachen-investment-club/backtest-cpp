#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <ctime>

#include "backtest-cpp/portfolio.h"
#include "backtest-cpp/types.h"

const uint32_t NQ_ID = 1;

// ============================================================================
// Test Fixture - Reusable setup for tests
// ============================================================================

class PortfolioTest : public ::testing::Test {
   protected:
    Portfolio* portfolio;

    // Runs before each test
    void SetUp() override {
        portfolio = new Portfolio({
            .initialCash = 100'000.0,
            .commission = 2.7,
            .leverage = 1.0  // Optional, defaults to 1.0
        });
    }

    // Runs after each test
    void TearDown() override {
        delete portfolio;
    }

    Bar createTestBar(uint32_t symbol_id, double price) {
        Bar bar;
        bar.symbol_id = symbol_id;
        bar.time = std::time(nullptr);
        bar.open = price;
        bar.high = price + 5;
        bar.low = price - 5;
        bar.close = price;
        bar.volume = 1000;
        return bar;
    }

    Bar createTestBar(double price) {
        return createTestBar(NQ_ID, price);
    }

    Order createTestOrder(uint32_t symbol_id, SignalType direction, double price, int quantity) {
        return Order{.time = std::time(nullptr),
                     .symbol_id = symbol_id,
                     .direction = direction,
                     .price = price,
                     .type = OrderType::MARKET,
                     .quantity = quantity};
    }
};

// ============================================================================
// Overdraft Tests
// ============================================================================

TEST_F(PortfolioTest, CheckOverdraftWithSufficientFunds) {
    Order order;
    order.symbol_id = NQ_ID;
    order.time = std::time(nullptr);
    order.price = 100.0;
    order.direction = SignalType::BUY;
    order.quantity = 10;
    order.type = OrderType::MARKET;

    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, CheckOverdraftWithInsufficientFunds) {
    Order order;
    order.time = std::time(nullptr);
    order.symbol_id = NQ_ID;
    order.direction = SignalType::BUY;
    order.price = 150.0;
    order.type = OrderType::MARKET;
    order.quantity = 1000;

    EXPECT_TRUE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, CheckOverdraftExactAmount) {
    Order order;
    order.symbol_id = NQ_ID;
    order.time = std::time(nullptr);
    order.price = 99.973;
    order.direction = SignalType::BUY;
    order.quantity = 1000;
    order.type = OrderType::MARKET;

    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

// ============================================================================
// Realized P&L Tests
// ============================================================================

TEST_F(PortfolioTest, NoTradesReturnsZeroRealizedPnL) {
    double pnl = portfolio->getRealizedPnL();
    EXPECT_DOUBLE_EQ(pnl, 0.0);
}

// ============================================================================
// Order Filtering Tests
// ============================================================================

TEST_F(PortfolioTest, GetAllOrdersReturnsEmptyWhenNoOrders) {
    int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    std::vector<Order> orders = portfolio->getAllOrders(now);
    EXPECT_TRUE(orders.empty());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PortfolioTest, ZeroQuantityOrderShouldNotOverdraft) {
    Order order;
    order.symbol_id = NQ_ID;
    order.time = std::time(nullptr);
    order.price = 1000000.0;  // Huge price
    order.direction = SignalType::BUY;
    order.quantity = 0;  // But zero quantity
    order.type = OrderType::MARKET;

    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, NegativeQuantityHandling) {
    Order order;
    order.symbol_id = NQ_ID;
    order.time = std::time(nullptr);
    order.price = 100.0;
    order.direction = SignalType::SELL;
    order.quantity = -10;  // Negative
    order.type = OrderType::MARKET;

    // Test evaluation directly without unused local variables
    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

// ============================================================================
// Basic Integration Checks (Decoupled from DataHandler)
// ============================================================================

TEST_F(PortfolioTest, CalculateEquityWithSyntheticData) {
    std::vector<Bar> currentBars(NQ_ID + 1);
    currentBars[NQ_ID] = createTestBar(NQ_ID, 100.0);

    double equity = portfolio->getTotalEquity(currentBars);
    EXPECT_DOUBLE_EQ(equity, 100000.0);
}

TEST_F(PortfolioTest, CInitialState) {
    std::vector<Bar> currentBars(NQ_ID + 1);
    currentBars[NQ_ID] = createTestBar(NQ_ID, 100.0);

    EXPECT_DOUBLE_EQ(portfolio->getTotalEquity(currentBars), 100000.0);
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, CNoPositionsReturnsZeroInvestedValue) {
    std::vector<Bar> currentBars(NQ_ID + 1);
    currentBars[NQ_ID] = createTestBar(NQ_ID, 100.0);

    double invested = portfolio->getInvestedValue(currentBars);
    EXPECT_DOUBLE_EQ(invested, 0.0);
}

TEST_F(PortfolioTest, CNoPositionsReturnsZeroUnrealizedPnL) {
    std::vector<Bar> currentBars(NQ_ID + 1);
    currentBars[NQ_ID] = createTestBar(NQ_ID, 100.0);

    double pnl = portfolio->getUnrealizedPnL(currentBars);
    EXPECT_DOUBLE_EQ(pnl, 0.0);
}

// ============================================================================
// NEW LONG POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, OpenLongPositionDeductsCash) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order, false);
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.30);
}

TEST_F(PortfolioTest, OpenLongPositionCreatesPosition) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order, false);

    const auto& positions = portfolio->getCurrentPositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions.at(NQ_ID).quantity, 10);
    EXPECT_DOUBLE_EQ(positions.at(NQ_ID).averagePrice, 100.0);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectInvestedValue) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 105.0);

    EXPECT_DOUBLE_EQ(portfolio->getInvestedValue(bars), 1050.0);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectUnrealizedPnL) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 105.0);

    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bars), 50.0 - 2.7);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectTotalEquity) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 105.0);

    EXPECT_DOUBLE_EQ(portfolio->getTotalEquity(bars), 100047.30);
}

// ============================================================================
// NEW SHORT POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, OpenShortPositionDeductsCash) {
    Order order = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order, false);
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.30);
}

TEST_F(PortfolioTest, OpenShortPositionCreatesPosition) {
    Order order = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order, false);

    const auto& positions = portfolio->getCurrentPositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions.at(NQ_ID).quantity, -10);
    EXPECT_DOUBLE_EQ(positions.at(NQ_ID).averagePrice, 100.0);
}

TEST_F(PortfolioTest, OpenShortPositionCorrectInvestedValue) {
    Order order = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 95.0);

    EXPECT_DOUBLE_EQ(portfolio->getInvestedValue(bars), 950.0);
}

TEST_F(PortfolioTest, OpenShortPositionCorrectUnrealizedPnL) {
    Order order = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 95.0);

    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bars), 50.0 - 2.7);
}

TEST_F(PortfolioTest, OpenShortPositionLosesMoneyWhenPriceRises) {
    Order order = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order, false);

    std::vector<Bar> bars(NQ_ID + 1);
    bars[NQ_ID] = createTestBar(NQ_ID, 105.0);

    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bars), -50.0 - 2.7);
}

// ============================================================================
// AVAILABLE CASH TESTS
// ============================================================================

TEST_F(PortfolioTest, AvailableCashAfterLongOpen) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.3);
}

TEST_F(PortfolioTest, AvailableCashAfterShortOpen) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::SELL, 100.0, 10);
    portfolio->executeOrder(openOrder, false);
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.3);
}

// ============================================================================
// CLOSING LONG POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, CloseLongPositionWithProfit) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder, true);
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 100094.6);
}

TEST_F(PortfolioTest, CloseLongPositionRemovesPosition) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder, true);

    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, CloseLongPositionCorrectRealizedPnL) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder, true);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, CloseLongPositionWithLoss) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::SELL, 95.0, -10);
    portfolio->executeOrder(closeOrder, true);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), -52.70);
}

TEST_F(PortfolioTest, CloseLongPositionTotalEquityConservation) {
    std::vector<Bar> bars1(NQ_ID + 1);
    bars1[NQ_ID] = createTestBar(NQ_ID, 100.0);

    double initialEquity = portfolio->getTotalEquity(bars1);

    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, true);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder, true);

    std::vector<Bar> bars2(NQ_ID + 1);
    bars2[NQ_ID] = createTestBar(NQ_ID, 110.0);
    double finalEquity = portfolio->getTotalEquity(bars2);

    EXPECT_DOUBLE_EQ(finalEquity, initialEquity + portfolio->getRealizedPnL());
}

// ============================================================================
// CLOSING SHORT POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, CloseShortPositionWithProfit) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::BUY, 90.0, 10);
    portfolio->executeOrder(closeOrder, true);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, CloseShortPositionWithLoss) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder, false);

    Order closeOrder = createTestOrder(NQ_ID, SignalType::BUY, 105.0, 10);
    portfolio->executeOrder(closeOrder, true);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), -52.70);
}

// ============================================================================
// REVERSING POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, ReverseLongToShort) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order reverseOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder, false);

    const auto& positions = portfolio->getCurrentPositions();
    EXPECT_EQ(positions.at(NQ_ID).quantity, -10);
    EXPECT_DOUBLE_EQ(positions.at(NQ_ID).averagePrice, 110.0);
}

TEST_F(PortfolioTest, ReverseLongToShortCorrectPnL) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    Order reverseOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder, false);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, ReverseLongToShortCorrectCash) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder, false);

    double cashAfterOpen = portfolio->getAvailableCash();

    Order reverseOrder = createTestOrder(NQ_ID, SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder, false);

    double expectedCash = cashAfterOpen + 1000.0 + 100 - 1100.0 - 2.7;
    EXPECT_NEAR(portfolio->getAvailableCash(), expectedCash, 0.01);
}

TEST_F(PortfolioTest, ReverseShortToLong) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder, false);

    Order reverseOrder = createTestOrder(NQ_ID, SignalType::BUY, 90.0, 20);
    portfolio->executeOrder(reverseOrder, false);

    const auto& positions = portfolio->getCurrentPositions();
    EXPECT_EQ(positions.at(NQ_ID).quantity, 10);
    EXPECT_DOUBLE_EQ(positions.at(NQ_ID).averagePrice, 90.0);
}

TEST_F(PortfolioTest, ReverseShortToLongCorrectPnL) {
    Order openOrder = createTestOrder(NQ_ID, SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder, false);

    Order reverseOrder = createTestOrder(NQ_ID, SignalType::BUY, 90.0, 20);
    portfolio->executeOrder(reverseOrder, false);

    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

// ============================================================================
// MULTIPLE TRADES TEST
// ============================================================================

TEST_F(PortfolioTest, MultipleTradesCorrectTotalPnL) {
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10), true);
    double pnl1 = portfolio->getRealizedPnL();

    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::SELL, 110.0, -10), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10), true);
    double pnl2 = portfolio->getRealizedPnL();

    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::SELL, 95.0, -10), true);
    double pnl3 = portfolio->getRealizedPnL();

    EXPECT_GT(pnl1, 0);
    EXPECT_GT(pnl2, pnl1);
    EXPECT_LT(pnl3, pnl2);
}

TEST_F(PortfolioTest, EquityConservationAcrossMultipleTrades) {
    std::vector<Bar> initialBars(NQ_ID + 1);
    initialBars[NQ_ID] = createTestBar(NQ_ID, 100.0);
    double initialEquity = portfolio->getTotalEquity(initialBars);

    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::SELL, 110.0, -20), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::BUY, 105.0, 20), false);
    portfolio->executeOrder(createTestOrder(NQ_ID, SignalType::SELL, 108.0, -10), true);

    std::vector<Bar> finalBars(NQ_ID + 1);
    finalBars[NQ_ID] = createTestBar(NQ_ID, 180.0);
    double finalEquity = portfolio->getTotalEquity(finalBars);
    double realizedPnL = portfolio->getRealizedPnL();

    EXPECT_NEAR(finalEquity, initialEquity + realizedPnL, 0.1);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(PortfolioTest, ZeroQuantityOrderDoesNothing) {
    double initialCash = portfolio->getAvailableCash();

    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 0);
    portfolio->executeOrder(order, false);

    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), initialCash);
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, InsufficientFundsBlocksOrder) {
    Order order = createTestOrder(NQ_ID, SignalType::BUY, 100.0, 10000);
    portfolio->executeOrder(order, false);

    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 100000.0);
}
