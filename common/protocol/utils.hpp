#pragma once

#include <string>


extern const std::size_t max_player_name_length;


bool validate_player_name(const std::string &player_name) noexcept;
