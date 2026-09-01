#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "backtest-cpp/symbol_dictionary.h"
#include "backtest-cpp/types.h"

struct BinaryCacheHeader {
    char magic[4];
    uint32_t version;
    uint32_t size_of_bar;
    uint32_t num_of_bars;
};
static_assert(
    std::is_trivially_copyable_v<BinaryCacheHeader>,
    "struct must not contain unique pointers, containers, custom constructors/destructors");
static_assert(sizeof(BinaryCacheHeader) == 16);
static_assert(alignof(BinaryCacheHeader) == 4);
static_assert(offsetof(BinaryCacheHeader, magic) == 0);
static_assert(offsetof(BinaryCacheHeader, version) == 4);
static_assert(offsetof(BinaryCacheHeader, size_of_bar) == 8);
static_assert(offsetof(BinaryCacheHeader, num_of_bars) == 12);
constexpr char CACHE_MAGIC[4] = {'B', 'T', 'C', 'P'};
constexpr uint32_t CACHE_FORMAT_VERSION = 1;

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
    std::vector<Bar>& getNextBars();
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
