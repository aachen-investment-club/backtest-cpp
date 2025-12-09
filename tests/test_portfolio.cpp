#include <gtest/gtest.h>

#include <ctime>

#include "data.h"
#include "portfolio.h"
#include "types.h"

// ============================================================================
// Test Fixture - Reusable setup for tests
// ============================================================================

class PortfolioTest : public ::testing::Test {
   protected:
    Portfolio* portfolio;
    DataHandler* data;

    // Runs before each test
    void SetUp() override {
        portfolio = new Portfolio({
            .initialCash = 100'000.0,
            .commission = 2.7,
            .leverage = 1.0  // Optional, defaults to 1.0
        });
        data = new DataHandler();
    }

    // Runs after each test
    void TearDown() override {
        delete portfolio;
        delete data;
    }

    // UPDATED: Helper function to create a test bar (with symbol parameter)
    Bar createTestBar(const std::string& symbol, double price) {
        Bar bar;
        bar.symbol = symbol;
        bar.time = std::time(nullptr);
        bar.open = price;
        bar.high = price + 5;
        bar.low = price - 5;
        bar.close = price;
        bar.volume = 1000;
        return bar;
    }

    // OVERLOAD: Keep original version for backward compatibility
    Bar createTestBar(double price) {
        return createTestBar("NQ", price);
    }

    // NEW: Helper function to create test orders
    Order createTestOrder(const std::string& symbol, SignalType direction, double price,
                          int quantity) {
        return Order{.time = std::time(nullptr),
                     .symbol = symbol,
                     .direction = direction,
                     .price = price,
                     .type = OrderType::MARKET,
                     .quantity = quantity};
    }
};

// ============================================================================
// Basic Portfolio Tests
// ============================================================================
// No data available, would trigger SEGFAULT

// ============================================================================
// Overdraft Tests
// ============================================================================

TEST_F(PortfolioTest, CheckOverdraftWithSufficientFunds) {
    // Order cost: 10 * $100 + $2.70 commission = $1,002.70
    // Available: $10,000
    // Should NOT overdraft

    Order order;
    order.symbol = "NQ";
    order.time = std::time(nullptr);
    order.price = 100.0;
    order.direction = SignalType::BUY;
    order.quantity = 10;
    order.type = OrderType::MARKET;

    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, CheckOverdraftWithInsufficientFunds) {
    // Order cost: 1000 * $150 + $2.70 = $150,002.70
    // Available: $100,000
    // Should overdraft

    Order order;
    order.time = std::time(nullptr);
    order.symbol = "NQ";
    order.direction = SignalType::BUY;
    order.price = 150.0;
    order.type = OrderType::MARKET;
    order.quantity = 1000;

    EXPECT_TRUE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, CheckOverdraftExactAmount) {
    // Order cost exactly equals available cash
    // 1000 * $99.973 + $2.70 = $100,000

    Order order;
    order.symbol = "NQ";
    order.time = std::time(nullptr);
    order.price = 99.973;
    order.direction = SignalType::BUY;
    order.quantity = 1000;
    order.type = OrderType::MARKET;

    // Should NOT overdraft (cost == available is okay)
    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

// ============================================================================
// Realized P&L Tests
// ============================================================================

TEST_F(PortfolioTest, NoTradesReturnsZeroRealizedPnL) {
    time_t now = std::time(nullptr);
    double pnl = portfolio->getRealizedPnL();
    EXPECT_DOUBLE_EQ(pnl, 0.0);
}

// Note: To test realized P&L properly, you'd need to implement
// handleBuyOrder() and handleSellOrder() first, which create Trade objects

// ============================================================================
// Order Filtering Tests
// ============================================================================

TEST_F(PortfolioTest, GetAllOrdersReturnsEmptyWhenNoOrders) {
    time_t now = std::time(nullptr);
    std::vector<Order> orders = portfolio->getAllOrders(now);
    EXPECT_TRUE(orders.empty());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PortfolioTest, ZeroQuantityOrderShouldNotOverdraft) {
    Order order;
    order.symbol = "NQ";
    order.time = std::time(nullptr);
    order.price = 1000000.0;  // Huge price
    order.direction = SignalType::BUY;
    order.quantity = 0;  // But zero quantity
    order.type = OrderType::MARKET;

    // Cost = 0 * 1000000 + 2.70 = $2.70
    // Should not overdraft
    EXPECT_FALSE(portfolio->checkOverdraft(order));
}

TEST_F(PortfolioTest, NegativeQuantityHandling) {
    // Test what happens with negative quantity (shouldn't happen, but test anyway)
    Order order;
    order.symbol = "NQ";
    order.time = std::time(nullptr);
    order.price = 100.0;
    order.direction = SignalType::SELL;
    order.quantity = -10;  // Negative!
    order.type = OrderType::MARKET;

    // Behavior depends on your implementation
    // For now, just ensure it doesn't crash
    bool result = portfolio->checkOverdraft(order);
    // Add expectation based on your intended behavior
}

// ============================================================================
// Integration Tests (require DataHandler with actual data)
// ============================================================================

// These tests need actual market data loaded
// You'd typically create a small test CSV file for this

class PortfolioWithDataTest : public ::testing::Test {
   protected:
    Portfolio* portfolio;
    DataHandler* data;

    void SetUp() override {
        portfolio = new Portfolio({.initialCash = 100000.0, .commission = 2.7});
        data = new DataHandler();

        // TODO: Create a small test CSV file and load it
        // data->loadCSV("tests/test_data.csv");
    }

    void TearDown() override {
        delete portfolio;
        delete data;
    }
};

// Example test that needs real data
TEST_F(PortfolioWithDataTest, CalculateEquityWithRealData) {
    // Load test data
    data->loadCSV("../data/Mini.csv");
    data->getNextBar();  // Move to first bar

    // Check initial equity
    double equity = portfolio->getTotalEquity(data->getCurrentBar());
    EXPECT_DOUBLE_EQ(equity, 100000.0);
}

TEST_F(PortfolioTest, CInitialState) {
    // Need to load data first!
    data->loadCSV("../data/Mini.csv");  // Load data
    data->getNextBar();                 // Move to first bar

    Bar currentBar = data->getCurrentBar();  // Get the bar

    EXPECT_DOUBLE_EQ(portfolio->getTotalEquity(currentBar), 100000.0);  // Pass bar
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, CNoPositionsReturnsZeroInvestedValue) {
    data->loadCSV("../data/Mini.csv");  // Load data
    data->getNextBar();

    Bar currentBar = data->getCurrentBar();                     // Get the bar
    double invested = portfolio->getInvestedValue(currentBar);  // Pass bar

    EXPECT_DOUBLE_EQ(invested, 0.0);
}

TEST_F(PortfolioTest, CNoPositionsReturnsZeroUnrealizedPnL) {
    data->loadCSV("../data/Mini.csv");  // Load data
    data->getNextBar();

    Bar currentBar = data->getCurrentBar();                // Get the bar
    double pnl = portfolio->getUnrealizedPnL(currentBar);  // Pass bar

    EXPECT_DOUBLE_EQ(pnl, 0.0);
}

// ============================================================================
// NEW LONG POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, OpenLongPositionDeductsCash) {
    // Open long: Buy 10 @ $100
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order);

    // Expected: 100,000 - (10 * 100 + 2.70) = 98,997.30
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.30);
}

TEST_F(PortfolioTest, OpenLongPositionCreatesPosition) {
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order);

    const auto& positions = portfolio->getCurrentPositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions.at("NQ").quantity, 10);
    EXPECT_DOUBLE_EQ(positions.at("NQ").averagePrice, 100.0);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectInvestedValue) {
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 105.0);  // Price moved to $105

    // Invested value = 10 * 105 = 1,050
    EXPECT_DOUBLE_EQ(portfolio->getInvestedValue(bar), 1050.0);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectUnrealizedPnL) {
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 105.0);

    // Unrealized P&L = 10 * (105 - 100) = 50 - 2.7 commission
    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bar), 50.0 - 2.7);
}

TEST_F(PortfolioTest, OpenLongPositionCorrectTotalEquity) {
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 105.0);

    // Total equity = Cash + Invested Value
    // = 98,997.30 + 1,050 = 100,047.30
    EXPECT_DOUBLE_EQ(portfolio->getTotalEquity(bar), 100047.30);
}

// ============================================================================
// NEW SHORT POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, OpenShortPositionDeductsCash) {
    // Open short: Sell 10 @ $100
    Order order = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order);

    // Expected: 100,000 - (10 * 100 + 2.70) = 98,997.30
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.30);
}

TEST_F(PortfolioTest, OpenShortPositionCreatesPosition) {
    Order order = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order);

    const auto& positions = portfolio->getCurrentPositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions.at("NQ").quantity, -10);  // Negative for short
    EXPECT_DOUBLE_EQ(positions.at("NQ").averagePrice, 100.0);
}

TEST_F(PortfolioTest, OpenShortPositionCorrectInvestedValue) {
    Order order = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 95.0);  // Price moved down to $95

    // Invested value = -10 * 95 = -950 → abs = 950
    EXPECT_DOUBLE_EQ(portfolio->getInvestedValue(bar), 950.0);
}

TEST_F(PortfolioTest, OpenShortPositionCorrectUnrealizedPnL) {
    Order order = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 95.0);

    // Unrealized P&L = -10 * (95 - 100) = -10 * -5 = 50 - 2.7 = 47.3 (profit)
    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bar), 50.0 - 2.7);
}

TEST_F(PortfolioTest, OpenShortPositionLosesMoneyWhenPriceRises) {
    Order order = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(order);

    Bar bar = createTestBar("NQ", 105.0);  // Price rises

    // Unrealized P&L = -10 * (105 - 100) = -10 * 5 = -50 (loss)
    EXPECT_DOUBLE_EQ(portfolio->getUnrealizedPnL(bar), -50.0 - 2.7);
}

// ============================================================================
// AVAILABLE CASH TESTS
// ============================================================================

TEST_F(PortfolioTest, AvailableCashAfterLongOpen) {
    // Open: Buy 10 @ $100
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    // Portfolio value 100'000
    // Cash should be availableCash_ - (10 * 100 + 2.7 ) =
    // = 98,997.30
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.3);
}

TEST_F(PortfolioTest, AvailableCashAfterShortOpen) {
    // Open: Buy 10 @ $100
    Order openOrder = createTestOrder("NQ", SignalType::SELL, 100.0, 10);
    portfolio->executeOrder(openOrder);

    // Portfolio value 100'000
    // Cash should be availableCash_ - (10 * 100 + 2.7 )=
    // = 98,997.30
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 98997.3);
}

// ============================================================================
// CLOSING LONG POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, CloseLongPositionWithProfit) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    double cashAfterOpen = portfolio->getAvailableCash();
    // std::cout << "Cash after open: " << cashAfterOpen << std::endl;

    Order closeOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder);

    double actualCash = portfolio->getAvailableCash();
    // std::cout << "Cash after close: " << actualCash << std::endl;

    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 100097.3);  // FIX
    // If fails, gtest prints: Expected: 100097.3, Actual: 99999.99
}

TEST_F(PortfolioTest, CloseLongPositionRemovesPosition) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    Order closeOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder);

    // Position should be removed
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, CloseLongPositionCorrectRealizedPnL) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    Order closeOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder);

    // Realized P&L = 10 * (110 - 100) - 2.70 = 97.30
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, CloseLongPositionWithLoss) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    Order closeOrder = createTestOrder("NQ", SignalType::SELL, 95.0, -10);
    portfolio->executeOrder(closeOrder);

    // P&L = 10 * (95 - 100) - 2.70 = -52.70
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), -52.70);
}

TEST_F(PortfolioTest, CloseLongPositionTotalEquityConservation) {
    Bar bar1 = createTestBar("NQ", 100.0);
    double initialEquity = portfolio->getTotalEquity(bar1);

    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    Order closeOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -10);
    portfolio->executeOrder(closeOrder);

    Bar bar2 = createTestBar("NQ", 110.0);
    double finalEquity = portfolio->getTotalEquity(bar2);

    // Final equity = Initial + Realized P&L
    EXPECT_DOUBLE_EQ(finalEquity, initialEquity + portfolio->getRealizedPnL());
}

// ============================================================================
// CLOSING SHORT POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, CloseShortPositionWithProfit) {
    // Open: Sell 10 @ $100
    Order openOrder = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder);

    // Close: Buy 10 @ $90
    Order closeOrder = createTestOrder("NQ", SignalType::BUY, 90.0, 10);
    portfolio->executeOrder(closeOrder);

    // P&L = -10 * (90 - 100) - 2.70 = 100 - 2.70 = 97.30
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, CloseShortPositionWithLoss) {
    Order openOrder = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder);

    Order closeOrder = createTestOrder("NQ", SignalType::BUY, 105.0, 10);
    portfolio->executeOrder(closeOrder);

    // P&L = -10 * (105 - 100) - 2.70 = -50 - 2.70 = -52.70
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), -52.70);
}

// ============================================================================
// REVERSING POSITION TESTS
// ============================================================================

TEST_F(PortfolioTest, ReverseLongToShort) {
    // Open: Buy 10 @ $100
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    // Reverse: Sell 20 @ $110 (closes 10 long, opens 10 short)
    Order reverseOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder);

    // Should now be short 10
    const auto& positions = portfolio->getCurrentPositions();
    EXPECT_EQ(positions.at("NQ").quantity, -10);
    EXPECT_DOUBLE_EQ(positions.at("NQ").averagePrice, 110.0);
}

TEST_F(PortfolioTest, ReverseLongToShortCorrectPnL) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    Order reverseOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder);

    // P&L from closing long = 10 * (110 - 100) - 2.70 = 97.30
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

TEST_F(PortfolioTest, ReverseLongToShortCorrectCash) {
    Order openOrder = createTestOrder("NQ", SignalType::BUY, 100.0, 10);
    portfolio->executeOrder(openOrder);

    double cashAfterOpen = portfolio->getAvailableCash();

    Order reverseOrder = createTestOrder("NQ", SignalType::SELL, 110.0, -20);
    portfolio->executeOrder(reverseOrder);

    // Cash flow:
    // 0. Original cash = 100'000 - 1000 - 2.7 = 98997.3
    // 1. Return collateral from long: 10 * 100 = 1,000
    // 2. Add P&L: 100
    // 3. Post collateral for short: 10 * 110 + 2.70 = 1,102.70
    // Final: cashAfterOpen + 1,000 + 97.30 - 1,102.70
    double expectedCash = cashAfterOpen + 1000.0 + 100 - 1100.0 - 2.7;

    EXPECT_NEAR(portfolio->getAvailableCash(), expectedCash, 0.01);
}

TEST_F(PortfolioTest, ReverseShortToLong) {
    // Open: Sell 10 @ $100
    Order openOrder = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder);

    // Reverse: Buy 20 @ $90 (closes 10 short, opens 10 long)
    Order reverseOrder = createTestOrder("NQ", SignalType::BUY, 90.0, 20);
    portfolio->executeOrder(reverseOrder);

    // Should now be long 10
    const auto& positions = portfolio->getCurrentPositions();
    EXPECT_EQ(positions.at("NQ").quantity, 10);
    EXPECT_DOUBLE_EQ(positions.at("NQ").averagePrice, 90.0);
}

TEST_F(PortfolioTest, ReverseShortToLongCorrectPnL) {
    Order openOrder = createTestOrder("NQ", SignalType::SELL, 100.0, -10);
    portfolio->executeOrder(openOrder);

    Order reverseOrder = createTestOrder("NQ", SignalType::BUY, 90.0, 20);
    portfolio->executeOrder(reverseOrder);

    // P&L from closing short = -10 * (90 - 100) - 2.70 = 100 - 2.70 = 97.30
    EXPECT_DOUBLE_EQ(portfolio->getRealizedPnL(), 97.30);
}

// ============================================================================
// MULTIPLE TRADES TEST
// ============================================================================

TEST_F(PortfolioTest, MultipleTradesCorrectTotalPnL) {
    // Trade 1: Long, profit
    portfolio->executeOrder(createTestOrder("NQ", SignalType::BUY, 100.0, 10));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::SELL, 110.0, -10));
    double pnl1 = portfolio->getRealizedPnL();

    // Trade 2: Short, profit
    portfolio->executeOrder(createTestOrder("NQ", SignalType::SELL, 110.0, -10));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::BUY, 100.0, 10));
    double pnl2 = portfolio->getRealizedPnL();

    // Trade 3: Long, loss
    portfolio->executeOrder(createTestOrder("NQ", SignalType::BUY, 100.0, 10));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::SELL, 95.0, -10));
    double pnl3 = portfolio->getRealizedPnL();

    // Total P&L should be cumulative
    EXPECT_GT(pnl1, 0);     // First trade profitable
    EXPECT_GT(pnl2, pnl1);  // Second adds to it
    EXPECT_LT(pnl3, pnl2);  // Third reduces it
}

TEST_F(PortfolioTest, EquityConservationAcrossMultipleTrades) {
    Bar initialBar = createTestBar("NQ", 100.0);
    double initialEquity = portfolio->getTotalEquity(initialBar);

    // Execute several trades
    portfolio->executeOrder(createTestOrder("NQ", SignalType::BUY, 100.0, 10));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::SELL, 110.0, -20));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::BUY, 105.0, 20));
    portfolio->executeOrder(createTestOrder("NQ", SignalType::SELL, 108.0, -10));

    Bar finalBar = createTestBar("NQ", 108.0);
    double finalEquity = portfolio->getTotalEquity(finalBar);
    double realizedPnL = portfolio->getRealizedPnL();

    // Final equity ≈ Initial equity + Realized P&L (within rounding)
    EXPECT_NEAR(finalEquity, initialEquity + realizedPnL, 0.1);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(PortfolioTest, ZeroQuantityOrderDoesNothing) {
    double initialCash = portfolio->getAvailableCash();

    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 0);
    portfolio->executeOrder(order);

    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), initialCash);
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());
}

TEST_F(PortfolioTest, InsufficientFundsBlocksOrder) {
    // Try to buy way more than we can afford
    Order order = createTestOrder("NQ", SignalType::BUY, 100.0, 10000);
    portfolio->executeOrder(order);

    // Should not create position
    EXPECT_TRUE(portfolio->getCurrentPositions().empty());

    // Cash should be unchanged
    EXPECT_DOUBLE_EQ(portfolio->getAvailableCash(), 100000.0);
}

// ============================================================================
// Main function (provided by gtest_main)
// ============================================================================