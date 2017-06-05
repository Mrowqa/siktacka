#include <common/protocol/utils.hpp>


constexpr std::size_t max_player_name_length = 64;
static inline constexpr bool is_allowed_player_name_character(char c) noexcept;


bool validate_player_name(const std::string &player_name) noexcept {
    if (player_name.length() > max_player_name_length) {
        return false;
    }

    for (auto c : player_name) {
        if (!is_allowed_player_name_character(c)) {
            return false;
        }
    }

    return true;
}


static inline constexpr bool is_allowed_player_name_character(char c) noexcept {
    return 33 <= c && c <= 126;
}
