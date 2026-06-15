#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class symbol_dictionary {
   public:
    uint32_t get_id(const std::string& symbol);
    std::string get_symbol(uint32_t id) const;
    size_t get_vector_size();

   private:
    std::unordered_map<std::string, uint32_t> string_to_id;
    std::unordered_map<uint32_t, std::string> id_to_string;
    uint32_t next_id = 0;
};