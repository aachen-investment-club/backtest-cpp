#include "backtest-cpp/data.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "backtest-cpp/symbol_dictionary.h"
#include "backtest-cpp/utils.h"
#include "csv.hpp"

void DataHandler::makeBinary(const std::string& csv_filepath, const std::string& binary_filepath,
                             uint32_t symbol_id, const std::string_view mode) {
    csv::CSVFormat format;
    format.trim({' ', '\t'});
    csv::CSVReader reader(csv_filepath, format);
    std::vector<Bar> bars;

    for (csv::CSVRow& row : reader) {
        Bar bar;
        bar.symbol_id = symbol_id;

        if (mode == "string") {
            auto tp = row["datetime"].get<std::chrono::system_clock::time_point>();
            bar.time =
                std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
        } else if (mode == "nanosecond") {
            bar.time = row["datetime"].get<int64_t>();
        } else {
            throw std::runtime_error(
                "Timestamp format of the csv file " + csv_filepath +
                " not recognized! Use string timestamp format (YYYY-MM-DDTHH:mm:ssZ) or nanosecond "
                "format (1609459200000000000LL)");
        }

        bar.open = row["open"].get<double>();
        bar.high = row["high"].get<double>();
        bar.low = row["low"].get<double>();
        bar.close = row["close"].get<double>();
        bar.volume = row["volume"].get<int32_t>();

        bars.push_back(bar);
    }

    std::ofstream out(binary_filepath, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open binary file for writing: " + binary_filepath);
    }

    // Safely write flat Bar vector to disk
    const auto bytes = bars.size() * sizeof(Bar);
    if (bytes > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("too much data to write");
    }

    out.write(reinterpret_cast<const char*>(bars.data()), static_cast<std::streamsize>(bytes));
    out.close();
}

std::vector<Bar> DataHandler::mapBinary(const std::string& binary_filepath) {
    std::ifstream in(binary_filepath, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("Failed to open binary file: " + binary_filepath);
    }

    // Safely read in binary file
    std::streamsize size = in.tellg();
    if (size == -1) {
        throw std::runtime_error("Stream error on file: " + binary_filepath);
    }
    if (size == 0) {
        return {};  // File has no data
    }
    in.seekg(0, std::ios::beg);

    const auto file_size = static_cast<std::size_t>(size);
    if (file_size % sizeof(Bar) != 0) {
        throw std::runtime_error(
            "Data file size has been corrupted - not a multiple of sizeof(Bar)");
    }

    std::size_t num_bars = file_size / sizeof(Bar);
    std::vector<Bar> bars(num_bars);

    // Read the ENTIRE file directly into the vector
    if (in.read(reinterpret_cast<char*>(bars.data()), size)) {
        return bars;
    } else {
        throw std::runtime_error("Error reading binary file: " + binary_filepath);
    }
}

void DataHandler::loadCSV(const std::string& csv_filepath, uint32_t symbol_id,
                          const std::string_view mode) {
    std::filesystem::path data_path(csv_filepath);
    std::string binary_filepath = data_path.replace_extension(".bin").string();

    // Check binary cache
    if (!std::filesystem::exists(binary_filepath)) {
        std::cout << "Generating Binary: " << binary_filepath << "...\n";
        makeBinary(csv_filepath, binary_filepath, symbol_id, mode);
    }

    // Ensure matrix is big enough  - necessary? prolly no. Good for performance? tbd.
    if (instrumentData_.size() <= symbol_id) {
        instrumentData_.resize(symbol_id + 1);
    }

    // Load rvalue returned by mapBinary() into internal instrumentData_ 2D vector
    instrumentData_[symbol_id] = mapBinary(binary_filepath);
    // TODO: make sure to add symbol of symbol_id to symDict before running with:
    // assign_and_save_id(symbol);

    // Guarantee chronological ascending order for k-way merge. This should be a redundant
    // just-in-case
    std::sort(instrumentData_[symbol_id].begin(), instrumentData_[symbol_id].end(),
              [](const Bar& a, const Bar& b) { return a.time < b.time; });
}

void DataHandler::loadAllCSVs(const std::string& directory, symbol_dictionary& symDict,
                              std::string_view mode = "string") {
    int counter = 0;

    // Register all symbols in the dictionary to get their ids
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory}) {
        if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".csv") {
            std::string filename = dir_entry.path().filename().string();
            symDict.assign_and_save_id(filename);  // Register the symbol
        }
    }

    // Load all data
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory}) {
        if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".csv") {
            std::string filename = dir_entry.path().filename().string();
            uint32_t id = symDict.get_id(filename);

            loadCSV(dir_entry.path().string(), id, mode);
            counter++;
        }
    }

    std::cout << "Successfully loaded " << counter << " files into memory.\n";

    synchronizeData();
}

void DataHandler::synchronizeData() {
    size_t num_symbols = instrumentData_.size();

    // Initialize our tracking arrays to match the number of symbols
    currentIndices_.assign(num_symbols, 0);

    // Initialize the market state vector with empty Bars
    currentMarketState_.assign(num_symbols, Bar{});
    updatedThisTick_.assign(num_symbols, false);
}

const std::vector<Bar>& DataHandler::getCurrentBars() const {
    if (currentMarketState_.empty()) {
        throw std::runtime_error("No bar has been processed yet. Call getNextBar() first.");
    }

    // if (currentIndex_ > instrumentData_.size()) {
    //     throw std::out_of_range("Current index out of bounds");
    // }

    return currentMarketState_;
}

const std::vector<Bar>& DataHandler::getNextBars() {
    int64_t max_time = std::numeric_limits<int64_t>::max();
    bool data_remains = false;

    // 1. Find the earliest next timestamp across all loaded assets
    for (size_t i = 0; i < instrumentData_.size(); ++i) {
        if (currentIndices_[i] < instrumentData_[i].size()) {
            int64_t next_time = instrumentData_[i][currentIndices_[i]].time;
            if (next_time < max_time) {
                max_time = next_time;
            }
            data_remains = true;
        }
    }

    if (!data_remains) {
        throw std::out_of_range("No more data available");
    }

    // Reset our tracker of what updated this tick
    std::fill(updatedThisTick_.begin(), updatedThisTick_.end(), false);

    // 2. Advance iterators ONLY for assets that have data at min_time
    for (size_t i = 0; i < instrumentData_.size(); ++i) {
        if (currentIndices_[i] < instrumentData_[i].size()) {
            // If this asset has a bar exactly at our target timestamp...
            if (instrumentData_[i][currentIndices_[i]].time == max_time) {
                // Update the current market state with this new bar
                currentMarketState_[i] = instrumentData_[i][currentIndices_[i]];
                updatedThisTick_[i] = true;

                // Move the pointer forward for next time
                currentIndices_[i]++;
            }
        }
    }

    // 3. Return the synchronized market state.
    // Notice how assets that didn't trade at min_time just kept their
    // old values in currentMarketState_! That is implicit forward-fill.
    return currentMarketState_;
}

bool DataHandler::hasMoreData() const {
    // Returns true if AT LEAST ONE symbol still has data left to process
    for (size_t i = 0; i < instrumentData_.size(); ++i) {
        if (currentIndices_[i] < instrumentData_[i].size()) {
            return true;
        }
    }
    return false;
}

void DataHandler::reset() {
    synchronizeData();
}

size_t DataHandler::size() const {
    size_t totalSize = 0;
    for (size_t id = 0; id < instrumentData_.size(); id++) {
        totalSize += instrumentData_[id].size();
    }
    return totalSize;
}