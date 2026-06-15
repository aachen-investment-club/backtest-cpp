#include <filesystem>
#include <fstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "backtest-cpp/data.h"
#include "backtest-cpp/utils.h"
#include "backtest-cpp/symbol_dictionary.h"

// be sure to add symbol of symbol_id to SymDict before running with: get_id(symbol);
void DataHandler::loadCSV(const std::string& filepath, uint32_t symbol_id,
                          const std::string_view mode) { 
    
    std::ifstream file(filepath);

    if (!file) {
        std::cerr << "Error: Could not open file: " << filepath << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);
    instrumentData_.reserve(instrumentData_.size() + getLineNumbers(filepath));

    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }

        if (row.size() >= 6) {  // Ensure we have all columns
            Bar bar;
            std::vector<Bar> symbolBarPair(symDict.get_vector_size());

            bar.symbol_id = symbol_id;
            bar.time = (mode == "nanoseconds") ? parseNanoseconds(row[0]) : parseDateTime(row[0]);
            // if(mode == "nanoseconds") {parseNanoseconds(row[0]);}
            // else {bar.time = parseDateTime(row[0]);}
            bar.open = std::stod(row[1]);
            bar.high = std::stod(row[2]);
            bar.low = std::stod(row[3]);
            bar.close = std::stod(row[4]);
            bar.volume = std::stol(row[5]);

            symbolBarPair[symbol_id] = bar;

            instrumentData_.push_back(symbolBarPair);  // Add to internal vector
        }
    }

    file.close();
    // std::cout << "Loaded " << instrumentData_.size() << " bars from " << filepath << std::endl;
    // // DEBUG
}

void DataHandler::loadAllCSVs(const std::string& directory) {
    int counter{0};
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory}) { // necessary to fix the size of symDict before loading CSVs happen
        if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".csv") {
            std::string filename = dir_entry.path().filename().string();
            symDict.get_id(filename);
        }
    }
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory}) {
        if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".csv") {
            std::string filename = dir_entry.path().filename().string();
            uint32_t id = symDict.get_id(filename);
            loadCSV(dir_entry.path().string(), id);
            counter++;
        }
    }
    std::cout << "Loaded " << counter << " csv files";
}

void DataHandler::synchronize(std::vector<std::vector<Bar> >&) {
    // Collects all unique timestamps from all loaded instruments
    // Sorts them chronologically
    // For each timestamp:

    // Check which instruments have bars at that timestamp
    // For instruments without data at that timestamp, use their most recent bar (forward-fill)
    // Store the complete map in synchronizedBars_
    return;
}

std::vector<Bar> DataHandler::getCurrentBars() const {
    if (currentIndex_ == 0) {
        throw std::runtime_error("No bar has been processed yet. Call getNextBar() first.");
    }

    if (currentIndex_ > instrumentData_.size()) {
        throw std::out_of_range("Current index out of bounds");
    }

    return instrumentData_[currentIndex_ - 1];
}

std::vector<Bar> DataHandler::getNextBars() {
    if (!hasMoreData()) {
        throw std::out_of_range("No more data available");
    }
    return instrumentData_[currentIndex_++];
}

bool DataHandler::hasMoreData() const {
    return currentIndex_ < instrumentData_.size();
}

void DataHandler::reset() {
    currentIndex_ = 0;
}

size_t DataHandler::size() const {
    return instrumentData_.size();
}