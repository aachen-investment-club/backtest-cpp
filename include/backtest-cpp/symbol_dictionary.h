#include <cstdint>
#include <unordered_map>>
#include <string>

class symbol_dictionary {
    public:
        uint32_t get_id(const std::string& symbol);
        std::string get_smbol(uint32_t id) const;

    private:
    std::unordered_map<std::string, uint32_t> string_to_id;
    std::unordered_map<uint32_t, std::string> id_to_string;
    uint32_t next_id = 0;
}; 