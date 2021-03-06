#pragma once


#include <common/protocol/GameEvent.hpp>

#include <deque>
#include <memory>


// Class for convenient serializing and deserializing
// multiple game events with game_id.
class MultipleGameEvent {
public:
    using Container = std::deque<std::unique_ptr<GameEvent>>;

    uint32_t game_id;
    Container events;

    // Gets container with already serialized GameEvents and offset from which
    // serialization should be started. Prepares packet with maximum possible
    // number of events and returns updated offset.
    // Must be called on valid struct.
    std::pair<std::string, uint32_t> prepare_packet_from_cache(
            const std::deque<std::string> &cache, uint32_t next_no) const noexcept;
    // Loads single binary packet updating class fields and returns true
    // if at least one GameEvent was successfully deserialized.
    // When error occurred, struct fields can be invalidated.
    bool deserialize(const std::string &data) noexcept;
    // Check if fields contain valid values.
    bool validate() const noexcept;
};


