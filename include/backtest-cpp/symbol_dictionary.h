#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class symbol_dictionary {
   public:
    uint32_t assign_and_save_id(const std::string& symbol);
    uint32_t get_id(std::string filename) const;
    std::string get_symbol(uint32_t id) const;
    size_t get_vector_size();
    std::vector<uint32_t> get_all_ids() const;

    // private:
    std::unordered_map<uint32_t, std::string> id_to_string_map_;
    std::unordered_map<std::string, uint32_t> string_to_id_map_;
    uint32_t last_id_ = 0;
};