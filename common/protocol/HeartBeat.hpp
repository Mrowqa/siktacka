#pragma once

#include <cstdint>
#include <string>


// HeartBeat sent by client to server.
// Fill fields directly in host bytes order.
struct HeartBeat final {
    uint64_t session_id;
    int8_t turn_direction;
    uint32_t next_expected_event_no;
    std::string player_name;

    // Prepares proper binary packet. Must be called on valid struct.
    std::string serialize() const noexcept;
    // Loads binary packet and returns if action succeeded.
    // When error occured, struct fields can be invalidated.
    bool deserialize(const std::string &data) noexcept;
    // Check if fields contain valid values.
    bool validate() const noexcept;
};
