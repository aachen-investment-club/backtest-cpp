#include <string>
#include <chrono>

// String utilities
std::string extractSymbolFromPath(const std::string& filepath);
uint64_t getLineNumbers(const std::string& filepath);

// Time utilities
time_t parseDateTime(const std::string& datetime_str);
std::string formatTimestamp(uint64_t timestamp);

// Math utilities
double round_to_tick(double price, double tick_size);