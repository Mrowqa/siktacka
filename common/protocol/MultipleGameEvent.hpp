#pragma once


#include <common/protocol/GameEvent.hpp>

#include <deque>
#include <memory>


// Class for convenient serializing and deserializing
// multiple game events with game_id.
class MultipleGameEvent {
public:
    using Container = std::deque<std::unique_ptr<GameEvent>>;
    using SerializedContainer = std::vector<std::string>;

    uint32_t game_id;
    Container events;

    // Prepares proper number of proper binary packets.
    // Must be called on valid struct.
    SerializedContainer serialize() const noexcept; // TODO serialize cached.
    // Loads single binary packet updating class fields and returns true
    // if at least one GameEvent was successfully deserialized.
    // When error occurred, struct fields can be invalidated.
    bool deserialize(const std::string &data) noexcept;
    // Check if fields contain valid values.
    bool validate() const noexcept;
};


