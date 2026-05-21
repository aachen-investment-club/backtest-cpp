#pragma once

#include <cstring>
#include <map>
#include <vector>

#include "backtest-cpp/symbol_dictionary.h"
#include "backtest-cpp/types.h"

class DataHandler {
   public:
    DataHandler() = default;
    DataHandler(symbol_dictionary& symDict) : symDict(symDict) {}

    void loadCSV(const std::string& filepath, uint32_t symbol_id);
    void loadAllCSVs(const std::string& directory);
    std::map<uint32_t, Bar> getNextBars();
    std::map<uint32_t, Bar> getCurrentBars() const;
    bool hasMoreData() const;
    void reset();
    void synchronize(std::vector<std::map<uint32_t, Bar>>& rawData);
    size_t size() const;

   private:
    symbol_dictionary& symDict;
    std::vector<std::map<uint32_t, Bar>> instrumentData_;  // Loaded, synced data
    size_t currentIndex_ = 0;                              // Current position in data
};