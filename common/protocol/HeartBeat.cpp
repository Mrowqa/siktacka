#include <common/protocol/HeartBeat.hpp>
#include <common/protocol/utils.hpp>

#include <cassert>
#include <cstring>
#include <endian.h>


static constexpr auto header_size = sizeof(HeartBeat) - sizeof(std::string);


std::string HeartBeat::serialize() const noexcept {
    assert(validate());
    std::string buffer(header_size + player_name.size(), '\0');

    // Simple way to generate binary packet in network byte order.
    auto buf_ptr = reinterpret_cast<HeartBeat*>(&buffer[0]);
    buf_ptr->session_id = htobe64(session_id);
    buf_ptr->turn_direction = turn_direction;
    buf_ptr->next_expected_event_no = htobe32(next_expected_event_no);
    memcpy(&buf_ptr->player_name, &player_name[0], player_name.size());

    return buffer;
}


bool HeartBeat::deserialize(const std::string &data) noexcept {
    if (data.length() < header_size || header_size + max_player_name_length < data.length()) {
        return false;
    }

    auto data_ptr = reinterpret_cast<const HeartBeat*>(&data[0]);
    session_id = be64toh(data_ptr->session_id);
    turn_direction = data_ptr->turn_direction;
    next_expected_event_no = be32toh(data_ptr->next_expected_event_no);

    auto player_name_length = data.length() - header_size;
    player_name.resize(player_name_length);
    memcpy(&player_name[0], &data_ptr->player_name, player_name_length);

    return validate();
}


bool HeartBeat::validate() const noexcept {
    if (turn_direction < -1 || 1 < turn_direction) {
        return false;
    }

    return validate_player_name(player_name);
}
