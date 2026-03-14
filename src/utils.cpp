#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "utils.h"
#include <iostream>

std::string extractSymbolFromPath(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}

time_t parseDateTime(const std::string& datetime_str) {
    std::tm tm = {};
    std::istringstream ss(datetime_str);

    // TODO pattern match to get the correct pattern for datetime
    // Parse format: "2008-01-02 06:00:00"
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Failed to parse datetime: " << datetime_str << std::endl;
        return 0;
    }

    // Convert to time_t
    return std::mktime(&tm);
}

uint64_t getLineNumbers(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    
    // Count newline characters
    uint64_t lineCount = std::count(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>(),
        '\n'
    );
    
    // Check if last line doesn't end with newline
    file.clear();
    file.seekg(-1, std::ios::end);
    char lastChar;
    if (file.get(lastChar) && lastChar != '\n') {
        lineCount++;
    }
    
    return lineCount;
}