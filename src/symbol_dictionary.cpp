#include "backtest-cpp/symbol_dictionary.h"

#include <vector>

uint32_t symbol_dictionary::assign_and_save_id(const std::string& symbol) {
    if (string_to_id_map_.find(symbol) != string_to_id_map_.end()) {
        return string_to_id_map_[symbol];
    }

    uint32_t new_id = ++last_id_;  // ids start from 1
    string_to_id_map_[symbol] = new_id;
    id_to_string_map_[new_id] = symbol;

    return new_id;
}

std::string symbol_dictionary::get_symbol(uint32_t id) const {
    return id_to_string_map_.at(id);
}

uint32_t symbol_dictionary::get_id(std::string filename) const {
    return string_to_id_map_.at(filename);
}

size_t symbol_dictionary::get_vector_size() {
    return id_to_string_map_.size() + 1;
}

std::vector<uint32_t> symbol_dictionary::get_all_ids() const {
    std::vector<uint32_t> id_vector;
    for (const auto& [id, symbol] : id_to_string_map_) {
        id_vector.push_back(id);
    }
    return id_vector;
}
