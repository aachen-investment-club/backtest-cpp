#pragma once

#include <cstring>
#include <map>
#include <string_view>
#include <vector>

#include "backtest-cpp/symbol_dictionary.h"
#include "backtest-cpp/types.h"

class DataHandler {
   public:
    DataHandler(symbol_dictionary& sym_dict) : symDict(sym_dict) {}

    void loadCSV(const std::string& filepath, uint32_t symbol_id,
                 const std::string_view mode = "string");
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
