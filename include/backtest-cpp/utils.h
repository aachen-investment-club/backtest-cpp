#include <chrono>
#include <string>
#include <string_view>

// String utilities
std::string extractSymbolFromPath(const std::string& filepath);
uint64_t getLineNumbers(const std::string& filepath);

// Time utilities
int64_t parseNanoseconds(std::string_view timestamp_str);
int64_t parseDateTime(const std::string& datetime_str);
std::string formatTimestamp(uint64_t timestamp);

// Math utilities
double round_to_tick(double price, double tick_size);