#include "backtest-cpp/symbol_dictionary.h"

uint32_t symbol_dictionary::get_id(const std::string& symbol) {
    if (string_to_id.find(symbol) != string_to_id.end()) {
        return string_to_id[symbol];
    }

    uint32_t new_id = ++last_id; // ids start from 1
    string_to_id[symbol] = new_id;
    id_to_string[new_id] = symbol;

    return new_id;
}

std::string symbol_dictionary::get_symbol(uint32_t id) const {
    return id_to_string.at(id);
}

size_t symbol_dictionary::get_vector_size() {
    return id_to_string.size() + 1;
}
