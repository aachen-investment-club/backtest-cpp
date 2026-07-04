#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "backtest-cpp/symbol_dictionary.h"
#include "backtest-cpp/types.h"

class DataHandler {
   public:
    DataHandler() = default;

    // Binary mapping
    void makeBinary(const std::string& csv_filepath, const std::string& binary_filepath,
                    uint32_t symbol_id, const std::string_view mode = "string");
    std::vector<Bar> mapBinary(const std::string& binary_filepath);

    void loadCSV(const std::string& csv_filepath, uint32_t symbol_id,
                 const std::string_view mode = "string");
    void loadAllCSVs(const std::string& directory, const std::string_view mode = "string");
    void loadAllCSVs(const std::string& directory, symbol_dictionary& symDict,
                     std::string_view mode);
    const std::vector<Bar>& getNextBars();
    const std::vector<Bar>& getCurrentBars() const;
    bool hasMoreData() const;
    void reset();
    void synchronizeData();
    size_t size() const;

   private:
    std::vector<std::vector<Bar>> instrumentData_;  // Loaded, synced data
    // size_t currentIndex_ = 0;

    // Tracks our current index for each symbol's vector.
    // currentIndices_[symbol_id] = the next index to read.
    std::vector<size_t> currentIndices_;

    // The "forward-filled" state of the market right now.
    // currentMarketState_[symbol_id] = the latest bar for that symbol.
    std::vector<Bar> currentMarketState_;

    // Keeps track of which symbols actually updated on the current tick (optional but very useful)
    std::vector<bool> updatedThisTick_;  // Current position in data
};
