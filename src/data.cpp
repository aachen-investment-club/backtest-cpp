#include "data.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

DataHandler::DataHandler() : currentIndex_(0) {}

time_t DataHandler::parseDateTime(const std::string& datetime_str) {
    std::tm tm = {};
    std::istringstream ss(datetime_str);

    // Parse format: "2008-01-02 06:00:00"
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Failed to parse datetime: " << datetime_str << std::endl;
        return 0;
    }

    // Convert to time_t
    return std::mktime(&tm);
}

void DataHandler::loadCSV(const std::string& filepath) {
    std::ifstream file(filepath);

    if (!file) {
        std::cerr << "Error: Could not open file: " << filepath << std::endl;
        return;
    }

    std::string line;

    std::getline(file, line);

    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }

        if (row.size() >= 6) {  // Ensure we have all columns
            Bar bar;

            bar.symbol = "NQ";
            bar.time = parseDateTime(row[0]);
            bar.open = std::stod(row[1]);
            bar.high = std::stod(row[2]);
            bar.low = std::stod(row[3]);
            bar.close = std::stod(row[4]);
            bar.volume = std::stol(row[5]);

            bars_.push_back(bar);  // Add to internal vector
        }
    }

    file.close();
    std::cout << "Loaded " << bars_.size() << " bars from " << filepath << std::endl;
}

Bar DataHandler::getCurrentBar() const {
    if (currentIndex_ == 0) {
        throw std::runtime_error("No bar has been processed yet. Call getNextBar() first.");
    }

    if (currentIndex_ > bars_.size()) {
        throw std::out_of_range("Current index out of bounds");
    }

    return bars_[currentIndex_ - 1];
}

Bar DataHandler::getNextBar() {
    if (!hasMoreData()) {
        throw std::out_of_range("No more data available");
    }
    return bars_[currentIndex_++];
}

bool DataHandler::hasMoreData() const {
    return currentIndex_ < bars_.size();
}

void DataHandler::reset() {
    currentIndex_ = 0;
}

size_t DataHandler::size() const {
    return bars_.size();
}