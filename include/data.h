#pragma once

#include <cstring>
#include <vector>
#include <map>

#include "types.h"

class DataHandler {
   public:
    DataHandler() = default;

    void loadCSV(const std::string& filepath, std::string symbol);
    void loadAllCSVs(const std::string& directory);
    std::map<std::string, Bar> getNextBars();
    std::map<std::string, Bar> getCurrentBars() const;
    bool hasMoreData() const;
    void reset();
    void synchronize(std::vector<std::map<std::string, Bar>>& rawData);
    size_t size() const;

   private:
    std::vector<std::map<std::string, Bar>>  instrumentData_; // Loaded, synced data
    size_t currentIndex_ = 0; // Current position in data
};