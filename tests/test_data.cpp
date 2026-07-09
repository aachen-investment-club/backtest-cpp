#include <gtest/gtest.h>

#include <ctime>
#include <filesystem>
#include <fstream>

#include "backtest-cpp/data.h"
#include "backtest-cpp/types.h"
const uint32_t NQ_ID = 1;  // IDs start from 1
const uint32_t SECOND_ID = 2;

class DataHandlerTest : public ::testing::Test {
   protected:
    symbol_dictionary symDict;
    DataHandler* data;
    std::string testFilePath;
    std::string binaryPath;

    void SetUp() override {
        data = new DataHandler;

        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        testFilePath = std::string("test_") + info->name() + ".csv";
        binaryPath = "./data/binary/test_" + std::string(info->name()) + ".bin";

        std::filesystem::create_directories("./data/binary");

        // assign_and_save_id INSERTS and returns the id. get_id() only looks up
        // and THROWS on a missing key -- that was the SetUp crash.
        symDict.assign_and_save_id("NQ");   // -> 1
        symDict.assign_and_save_id("SEC");  // -> 2
    }

    void TearDown() override {
        delete data;
        std::remove(testFilePath.c_str());
        std::filesystem::remove(binaryPath);  // kill cached binary between tests
    }

    // NOTE: header is "datetime" to match data.cpp's row["datetime"] lookup.
    void createTestCSV(int numBars) {
        std::ofstream file(testFilePath);
        file << "datetime,open,high,low,close,volume\n";
        for (int i = 0; i < numBars; i++) {
            double basePrice = 3700.0 + i;
            file << (1609459200 + i * 3600) << "," << basePrice << "," << (basePrice + 10) << ","
                 << (basePrice - 10) << "," << (basePrice + 5) << "," << (100000 + i * 1000)
                 << "\n";
        }
        file.close();
    }

    void createInvalidCSV() {
        std::ofstream file(testFilePath);
        file << "datetime,open,high,low,close,volume\n";
        file << "invalid,data,here,not,numbers,bad\n";
        file.close();
    }
};

TEST_F(DataHandlerTest, InitialState) {
    EXPECT_EQ(data->size(), 0);
    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, LoadCSVWithValidData) {
    createTestCSV(5);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 5);
    EXPECT_TRUE(data->hasMoreData());
}

TEST_F(DataHandlerTest, LoadCSVMultipleBars) {
    createTestCSV(100);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 100);
}

TEST_F(DataHandlerTest, LoadCSVSetsCorrectValues) {
    createTestCSV(1);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    Bar bar = data->getNextBars()[NQ_ID];
    EXPECT_EQ(bar.symbol_id, NQ_ID);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar.open), 3700.0);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar.high), 3710.0);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar.low), 3690.0);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar.close), 3705.0);
    EXPECT_EQ(bar.volume, 100000);
}

TEST_F(DataHandlerTest, LoadCSVEmptyFile) {
    std::ofstream file(testFilePath);
    file << "datetime,open,high,low,close,volume\n";
    file.close();
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 0);
}

TEST_F(DataHandlerTest, LoadCSVSkipsInvalidRows) {
    std::ofstream file(testFilePath);
    file << "datetime,open,high,low,close,volume\n";
    file << "1609459200,3700,3710,3690,3705,100000\n";
    file << "invalid,data,row\n";
    file << "1609462800,3715,3725,3705,3720,101000\n";
    file.close();
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 2);
}

TEST_F(DataHandlerTest, GetNextBarReturnsSequentially) {
    createTestCSV(3);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    Bar bar1 = data->getNextBars()[NQ_ID];
    Bar bar2 = data->getNextBars()[NQ_ID];
    Bar bar3 = data->getNextBars()[NQ_ID];
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar1.close), 3705.0);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar2.close), 3706.0);
    EXPECT_DOUBLE_EQ(priceIntToDouble(bar3.close), 3707.0);
}

TEST_F(DataHandlerTest, GetNextBarAdvancesIndex) {
    createTestCSV(5);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_TRUE(data->hasMoreData());
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_TRUE(data->hasMoreData());
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_TRUE(data->hasMoreData());
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, GetNextBarThrowsWhenNoMoreData) {
    createTestCSV(1);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_THROW(static_cast<void>(data->getNextBars().at(NQ_ID)), std::out_of_range);
}

TEST_F(DataHandlerTest, GetNextBarOnEmptyDataThrows) {
    EXPECT_THROW(static_cast<void>(data->getNextBars().at(NQ_ID)), std::out_of_range);
}

TEST_F(DataHandlerTest, GetCurrentBarAfterGetNext) {
    createTestCSV(3);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    Bar next = data->getNextBars().at(NQ_ID);
    Bar current = data->getCurrentBars().at(NQ_ID);
    EXPECT_EQ(next.close, current.close);
    EXPECT_DOUBLE_EQ(priceIntToDouble(current.close), 3705.0);
}

TEST_F(DataHandlerTest, GetCurrentBarDoesNotAdvanceIndex) {
    createTestCSV(3);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    static_cast<void>(data->getNextBars().at(NQ_ID));
    Bar current1 = data->getCurrentBars()[NQ_ID];
    Bar current2 = data->getCurrentBars()[NQ_ID];
    Bar current3 = data->getCurrentBars()[NQ_ID];
    EXPECT_EQ(current1.close, current2.close);
    EXPECT_EQ(current2.close, current3.close);
}

TEST_F(DataHandlerTest, GetCurrentBarOnEmptyDataHandlesGracefully) {
    // Intentionally empty.
}

TEST_F(DataHandlerTest, HasMoreDataInitiallyTrue) {
    createTestCSV(5);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_TRUE(data->hasMoreData());
}

TEST_F(DataHandlerTest, HasMoreDataFalseWhenExhausted) {
    createTestCSV(2);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, HasMoreDataFalseWhenEmpty) {
    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, ResetAllowsReprocessing) {
    createTestCSV(3);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_FALSE(data->hasMoreData());
    data->reset();
    EXPECT_TRUE(data->hasMoreData());
    EXPECT_EQ(data->size(), 3);
}

TEST_F(DataHandlerTest, ResetRestartsFromBeginning) {
    createTestCSV(3);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    Bar firstBar = data->getNextBars().at(NQ_ID);
    static_cast<void>(data->getNextBars().at(NQ_ID));
    data->reset();
    Bar firstBarAgain = data->getNextBars().at(NQ_ID);
    EXPECT_EQ(firstBar.close, firstBarAgain.close);
}

TEST_F(DataHandlerTest, SizeReturnsCorrectCount) {
    createTestCSV(42);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 42);
}

TEST_F(DataHandlerTest, SizeUnchangedByGetNextBar) {
    createTestCSV(5);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    size_t initialSize = data->size();
    static_cast<void>(data->getNextBars().at(NQ_ID));
    static_cast<void>(data->getNextBars().at(NQ_ID));
    EXPECT_EQ(data->size(), initialSize);
}

TEST_F(DataHandlerTest, CompleteWorkflow) {
    createTestCSV(10);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(data->hasMoreData());
        Bar bar = data->getNextBars().at(NQ_ID);
        EXPECT_EQ(bar.symbol_id, NQ_ID);
    }
    EXPECT_TRUE(data->hasMoreData());
    data->reset();
    Bar firstBar = data->getNextBars().at(NQ_ID);
    EXPECT_DOUBLE_EQ(priceIntToDouble(firstBar.close), 3705.0);
}

TEST_F(DataHandlerTest, LoadMultipleFiles) {
    createTestCSV(5);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 5);

    std::string secondFile = "test_LoadMultipleFiles_2.csv";
    std::ofstream file(secondFile);
    file << "datetime,open,high,low,close,volume\n";
    file << "1609459200,4000,4010,3990,4005,200000\n";
    file.close();

    data->loadCSV(secondFile, SECOND_ID, "nanosecond");
    EXPECT_EQ(data->size(), 6);

    std::remove(secondFile.c_str());
    std::filesystem::remove("./data/binary/test_LoadMultipleFiles_2.bin");
}

TEST_F(DataHandlerTest, VeryLargeDataset) {
    createTestCSV(10000);
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    EXPECT_EQ(data->size(), 10000);
    for (int i = 0; i < 10000; i++) {
        ASSERT_TRUE(data->hasMoreData());
        static_cast<void>(data->getNextBars().at(NQ_ID));
    }
    EXPECT_FALSE(data->hasMoreData());
}

TEST_F(DataHandlerTest, ZeroVolumeBars) {
    std::ofstream file(testFilePath);
    file << "datetime,open,high,low,close,volume\n";
    file << "1609459200,3700,3710,3690,3705,0\n";
    file.close();
    data->loadCSV(testFilePath, NQ_ID, "nanosecond");
    Bar bar = data->getNextBars().at(NQ_ID);
    EXPECT_EQ(bar.volume, 0);
}