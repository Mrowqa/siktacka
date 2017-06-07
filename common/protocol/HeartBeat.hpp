#pragma once

#include <cstdint>
#include <string>


// HeartBeat sent by client to server.
// Class for convenient serializing and deserializing packets.
struct HeartBeat final {
    uint64_t session_id = 0;
    int8_t turn_direction = 0;
    uint32_t next_expected_event_no = 0;
    std::string player_name;

    // Prepares proper binary packet. Must be called on valid struct.
    std::string serialize() const noexcept;
    // Loads binary packet updating struct fields
    // and returns true if action succeeded.
    // When error occurred, struct fields can be invalidated.
    bool deserialize(const std::string &data) noexcept;
    // Check if fields contain valid values.
    bool validate() const noexcept;
};
