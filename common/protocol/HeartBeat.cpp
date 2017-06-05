#include <common/protocol/HeartBeat.hpp>
#include <common/protocol/utils.hpp>

#include <cassert>
#include <cstring>
#include <endian.h>


static constexpr auto header_size = sizeof(uint64_t) + sizeof(int8_t) + sizeof(uint32_t);


std::string HeartBeat::serialize() const noexcept {
    assert(validate());
    std::string buffer(header_size + player_name.size(), '\0');

    *reinterpret_cast<uint64_t*>(&buffer[0]) = htobe64(session_id);
    *reinterpret_cast<uint8_t*> (&buffer[8]) = turn_direction;
    *reinterpret_cast<uint32_t*>(&buffer[9]) = htobe32(next_expected_event_no);
    memcpy(&buffer[13], &player_name[0], player_name.size());

    return buffer;
}


bool HeartBeat::deserialize(const std::string &data) noexcept {
    if (data.length() < header_size || header_size + max_player_name_length < data.length()) {
        return false;
    }

    session_id = be64toh(*reinterpret_cast<const uint64_t*>(&data[0]));
    turn_direction = *reinterpret_cast<const uint8_t*>(&data[8]);
    next_expected_event_no = be32toh(*reinterpret_cast<const uint32_t*>(&data[9]));

    auto player_name_length = data.length() - header_size;
    player_name.resize(player_name_length);
    memcpy(&player_name[0], &data[13], player_name_length);

    return validate();
}


bool HeartBeat::validate() const noexcept {
    if (turn_direction < -1 || 1 < turn_direction) {
        return false;
    }

    return validate_player_name(player_name);
}
