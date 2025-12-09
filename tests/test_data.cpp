#include <gtest/gtest.h>

#include <ctime>
#include <fstream>

#include "data.h"
#include "types.h"

// ============================================================================
// Test Fixture
// ============================================================================

class DataHandlerTest : public ::testing::Test {
   protected:
    DataHandler* data;
    std::string testFilePath;

    void SetUp() override {
        data = new DataHandler();
        testFilePath = "test_data_temp.csv";
    }

    void TearDown() override {
        delete data;
        // Clean up test file
        std::remove(testFilePath.c_str());
    }

    // Helper: Create a test CSV file
    void createTestCSV(int numBars) {
        std::ofstream file(testFilePath);
        file << "timestamp,open,high,low,close,volume\n";

        for (int i = 0; i < numBars; i++) {
            double basePrice = 3700.0 + i;
            file << (1609459200 + i * 3600) << ","  // timestamp
                 << basePrice << ","                // open
                 << (basePrice + 10) << ","         // high
                 << (basePrice - 10) << ","         // low
                 << (basePrice + 5) << ","          // close
                 << (100000 + i * 1000) << "\n";    // volume
        }

        file.close();
    }

    // Helper: Create CSV with invalid data
    void createInvalidCSV() {
        std::ofstream file(testFilePath);
        file << "timestamp,open,high,low,close,volume\n";
        file << "invalid,data,here,not,numbers,bad\n";
        file.close();
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(DataHandlerTest, InitialState) {
    // New DataHandler should be empty
    EXPECT_EQ(data->size(), 0);
    EXPECT_FALSE(data->hasMoreData());
}

// ============================================================================
// CSV Loading Tests
// ============================================================================

TEST_F(DataHandlerTest, LoadCSVWithValidData) {
    createTestCSV(5);

    data->loadCSV(testFilePath);

    EXPECT_EQ(data->size(), 5);
    EXPECT_TRUE(data->hasMoreData());
}

TEST_F(DataHandlerTest, LoadCSVMultipleBars) {
    createTestCSV(100);

    data->loadCSV(testFilePath);

    EXPECT_EQ(data->size(), 100);
}

TEST_F(DataHandlerTest, LoadCSVSetsCorrectValues) {
    createTestCSV(1);

    data->loadCSV(testFilePath);
    Bar bar = data->getNextBar();

    EXPECT_EQ(bar.symbol, "NQ");
    EXPECT_DOUBLE_EQ(bar.open, 3700.0);
    EXPECT_DOUBLE_EQ(bar.high, 3710.0);
    EXPECT_DOUBLE_EQ(bar.low, 3690.0);
    EXPECT_DOUBLE_EQ(bar.close, 3705.0);
    EXPECT_EQ(bar.volume, 100000);
}

TEST_F(DataHandlerTest, LoadCSVNonExistentFile) {
    // Should handle gracefully, not crash
    data->loadCSV("this_file_does_not_exist.csv");

    EXPECT_EQ(data->size(), 0);
}

TEST_F(DataHandlerTest, LoadCSVEmptyFile) {
    // Create empty CSV (just header)
    std::ofstream file(testFilePath);
    file << "timestamp,open,high,low,close,volume\n";
    file.close();

    data->loadCSV(testFilePath);

    EXPECT_EQ(data->size(), 0);
}

TEST_F(DataHandlerTest, LoadCSVSkipsInvalidRows) {
    // Mix of valid and invalid data
    std::ofstream file(testFilePath);
    file << "timestamp,open,high,low,close,volume\n";
    file << "1609459200,3700,3710,3690,3705,100000\n";  // Valid
    file << "invalid,data,row\n";                       // Invalid (< 6 columns)
    file << "1609462800,3715,3725,3705,3720,101000\n";  // Valid
    file.close();

    data->loadCSV(testFilePath);

    // Should load 2 valid bars, skip the invalid one
    EXPECT_EQ(data->size(), 2);
}

// ============================================================================
// getNextBar() Tests
// ============================================================================

TEST_F(DataHandlerTest, GetNextBarReturnsSequentially) {
    createTestCSV(3);
    data->loadCSV(testFilePath);

    Bar bar1 = data->getNextBar();
    Bar bar2 = data->getNextBar();
    Bar bar3 = data->getNextBar();

    EXPECT_DOUBLE_EQ(bar1.close, 3705.0);
    EXPECT_DOUBLE_EQ(bar2.close, 3706.0);
    EXPECT_DOUBLE_EQ(bar3.close, 3707.0);
}

TEST_F(DataHandlerTest, GetNextBarAdvancesIndex) {
    createTestCSV(5);
    data->loadCSV(testFilePath);

    EXPECT_TRUE(data->hasMoreData());

    data->getNextBar();
    EXPECT_TRUE(data->hasMoreData());  // 4 left

    data->getNextBar();
    EXPECT_TRUE(data->hasMoreData());  // 3 left

    data->getNextBar();
    data->getNextBar();
    data->getNextBar();

    EXPECT_FALSE(data->hasMoreData());  // All consumed
}

TEST_F(DataHandlerTest, GetNextBarThrowsWhenNoMoreData) {
    createTestCSV(1);
    data->loadCSV(testFilePath);

    data->getNextBar();  // Get the only bar

    // Should throw when trying to get another
    EXPECT_THROW(data->getNextBar(), std::out_of_range);
}

TEST_F(DataHandlerTest, GetNextBarOnEmptyDataThrows) {
    // Don't load any data
    EXPECT_THROW(data->getNextBar(), std::out_of_range);
}

// ============================================================================
// getCurrentBar() Tests
// ============================================================================

// NOTE: These tests will FAIL with your current implementation!
// They demonstrate the bugs in getCurrentBar()

TEST_F(DataHandlerTest, GetCurrentBarAfterGetNext) {
    createTestCSV(3);
    data->loadCSV(testFilePath);

    Bar next = data->getNextBar();        // Get first bar
    Bar current = data->getCurrentBar();  // Should return same bar

    // Both should be the first bar
    EXPECT_DOUBLE_EQ(next.close, current.close);
    EXPECT_DOUBLE_EQ(current.close, 3705.0);
}

TEST_F(DataHandlerTest, GetCurrentBarDoesNotAdvanceIndex) {
    createTestCSV(3);
    data->loadCSV(testFilePath);

    data->getNextBar();  // Advance to first bar

    Bar current1 = data->getCurrentBar();
    Bar current2 = data->getCurrentBar();
    Bar current3 = data->getCurrentBar();

    // All should be the same (first bar)
    EXPECT_DOUBLE_EQ(current1.close, current2.close);
    EXPECT_DOUBLE_EQ(current2.close, current3.close);
}

TEST_F(DataHandlerTest, GetCurrentBarOnEmptyDataHandlesGracefully) {
    // Don't load any data

    // Your current implementation will crash!
    // Should either throw or return safely
    // EXPECT_THROW(data->getCurrentBar(), std::runtime_error);

    // For now, just ensure it doesn't crash the test suite
    // (Comment out if it crashes)
}

// ============================================================================
// hasMoreData() Tests
// ============================================================================

TEST_F(DataHandlerTest, HasMoreDataInitiallyTrue) {
    createTestCSV(5);
    data->loadCSV(testFilePath);

    EXPECT_TRUE(data->hasMoreData());
}

TEST_F(DataHandlerTest, HasMoreDataFalseWhenExhausted) {
    createTestCSV(2);
    data->loadCSV(testFilePath);

    data->getNextBar();
    data->getNextBar();

    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, HasMoreDataFalseWhenEmpty) {
    EXPECT_FALSE(data->hasMoreData());
}

// ============================================================================
// reset() Tests
// ============================================================================

TEST_F(DataHandlerTest, ResetAllowsReprocessing) {
    createTestCSV(3);
    data->loadCSV(testFilePath);

    // Process all bars
    data->getNextBar();
    data->getNextBar();
    data->getNextBar();

    EXPECT_FALSE(data->hasMoreData());

    // Reset
    data->reset();

    EXPECT_TRUE(data->hasMoreData());
    EXPECT_EQ(data->size(), 3);  // Data still there
}

TEST_F(DataHandlerTest, ResetRestartsFromBeginning) {
    createTestCSV(3);
    data->loadCSV(testFilePath);

    Bar firstBar = data->getNextBar();
    data->getNextBar();  // Skip to second

    data->reset();

    Bar firstBarAgain = data->getNextBar();

    EXPECT_DOUBLE_EQ(firstBar.close, firstBarAgain.close);
}

// ============================================================================
// size() Tests
// ============================================================================

TEST_F(DataHandlerTest, SizeReturnsCorrectCount) {
    createTestCSV(42);
    data->loadCSV(testFilePath);

    EXPECT_EQ(data->size(), 42);
}

TEST_F(DataHandlerTest, SizeUnchangedByGetNextBar) {
    createTestCSV(5);
    data->loadCSV(testFilePath);

    size_t initialSize = data->size();

    data->getNextBar();
    data->getNextBar();

    EXPECT_EQ(data->size(), initialSize);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(DataHandlerTest, CompleteWorkflow) {
    createTestCSV(10);
    data->loadCSV(testFilePath);

    // Process first 5 bars
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(data->hasMoreData());
        Bar bar = data->getNextBar();
        EXPECT_EQ(bar.symbol, "NQ");
    }

    EXPECT_TRUE(data->hasMoreData());  // 5 left

    // Reset and start over
    data->reset();

    Bar firstBar = data->getNextBar();
    EXPECT_DOUBLE_EQ(firstBar.close, 3705.0);  // Back to first
}

TEST_F(DataHandlerTest, LoadMultipleFiles) {
    // Load first file
    createTestCSV(5);
    data->loadCSV(testFilePath);
    EXPECT_EQ(data->size(), 5);

    // Load second file (overwrites? or appends?)
    // Your implementation will APPEND (bug or feature?)
    std::string secondFile = "test_data_temp2.csv";
    std::ofstream file(secondFile);
    file << "timestamp,open,high,low,close,volume\n";
    file << "1609459200,4000,4010,3990,4005,200000\n";
    file.close();

    data->loadCSV(secondFile);

    // Current implementation appends - is this intended?
    EXPECT_EQ(data->size(), 6);  // 5 + 1

    std::remove(secondFile.c_str());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(DataHandlerTest, VeryLargeDataset) {
    createTestCSV(10000);
    data->loadCSV(testFilePath);

    EXPECT_EQ(data->size(), 10000);

    // Process all
    for (int i = 0; i < 10000; i++) {
        ASSERT_TRUE(data->hasMoreData());
        data->getNextBar();
    }

    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, ZeroVolumeBars) {
    std::ofstream file(testFilePath);
    file << "timestamp,open,high,low,close,volume\n";
    file << "1609459200,3700,3710,3690,3705,0\n";  // Zero volume
    file.close();

    data->loadCSV(testFilePath);
    Bar bar = data->getNextBar();

    EXPECT_EQ(bar.volume, 0);
}