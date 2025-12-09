#pragma once

#include <cstring>
#include <vector>

#include "types.h"

class DataHandler {
   public:
    DataHandler();  // Constructor

    time_t parseDateTime(const std::string& datetime_str);
    void loadCSV(const std::string& filepath);
    Bar getNextBar();
    Bar getCurrentBar() const;
    bool hasMoreData() const;
    void reset();
    size_t size() const;

   private:
    std::vector<Bar> bars_;  // All loaded bars
    size_t currentIndex_;    // Current position in data
};