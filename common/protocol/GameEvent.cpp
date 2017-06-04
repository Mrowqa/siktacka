#include "GameEvent.hpp"
#include <common/protocol/utils.hpp>
#include <common/network/UdpSocket.hpp>
#include <common/utils.hpp>

#include <cassert>


// ------------------------------------------------------------------------------------------------
//                                         GameEvent
// ------------------------------------------------------------------------------------------------
std::string GameEvent::serialize(GameEvent::Format fmt) const noexcept {
    assert(validate(fmt));

    return fmt == GameEvent::Format::Binary ? serialize_binary() : serialize_text();
}


bool GameEvent::deserialize(GameEvent::Format fmt, const std::string &data) noexcept {
    if (fmt != GameEvent::Format::Binary) {
        throw unimplemented_error("GameEvent: deserialize does not support Format::Text");
    }

    // deserialize... TODO
}


bool GameEvent::validate(GameEvent::Format fmt) const noexcept {
    switch (type) {
        case Type::NewGame:  return new_game_data.validate(fmt);
        case Type::Pixel:    return pixel_data.validate(fmt);
        case Type::PlayerEliminated:
                             return player_eliminated_data.validate(fmt);
        case Type::GameOver: return fmt == Format::Binary;
        default:             return false;
    }
}


std::string GameEvent::serialize_binary() const noexcept {
    return std::string();  // TODO
}


std::string GameEvent::serialize_text() const noexcept {
    assert(type == Type::NewGame || type == Type::Pixel || type == Type::PlayerEliminated);

    std::ostringstream str;

    switch (type) {
        case Type::NewGame:
            str << "NEW_GAME " << new_game_data.maxx << ' ' << new_game_data.maxy;
            for (const auto &name : new_game_data.player_names) {
                str << ' ' << name;
            }
            break;

        case Type::Pixel:
            str << "PIXEL " << pixel_data.x << ' ' << pixel_data.y << ' ' << pixel_data.player_name;
            break;

        case Type::PlayerEliminated:
            str << "PLAYER_ELIMINATED " << player_eliminated_data.player_name;
            break;

        default:
            assert(false);
            break;
    }

    return str.str();
}


// ------------------------------------------------------------------------------------------------
//                                    GameEvent::NewGameData
// ------------------------------------------------------------------------------------------------
const std::size_t GameEvent::NewGameData::names_capacity =
        max_datagram_size - (
                sizeof(uint32_t)        // game_id
                + sizeof(uint32_t)      // len
                + sizeof(uint32_t)      // event_no
                + sizeof(Type)          // type
                + sizeof(uint32_t) * 2  // max{x,y}
                + sizeof(uint32_t)      // crc32
);

std::size_t GameEvent::NewGameData::calculate_used_names_capacity() const noexcept {
    std::size_t result = 0;

    for (const auto &name : player_names) {
        result += name.size() + 1;
    }

    return result;
}


std::size_t GameEvent::NewGameData::serialize_binary(std::string &buf, std::size_t offset) const noexcept {
    // TODO
}


bool GameEvent::NewGameData::deserialize_binary(const std::string &data, std::size_t offset,
                                                std::size_t size) noexcept {
    return false;  // TODO
}


bool GameEvent::NewGameData::validate(GameEvent::Format fmt) const noexcept {
    if (calculate_used_names_capacity() > names_capacity) {
        return false;
    }

    for (const auto &name : player_names) {
        if (!validate_player_name(name)) {
            return false;
        }
    }

    return true;
}


// ------------------------------------------------------------------------------------------------
//                                     GameEvent::PixelData
// ------------------------------------------------------------------------------------------------
std::size_t GameEvent::PixelData::serialize_binary(std::string &buf, std::size_t offset) const noexcept {
    // TODO
}


bool GameEvent::PixelData::deserialize_binary(const std::string &data, std::size_t offset,
                                              std::size_t size) noexcept {
    return false;  // TODO
}


bool GameEvent::PixelData::validate(GameEvent::Format fmt) const noexcept {
    if (fmt == Format::Text) {
        if (player_name.empty() || !validate_player_name(player_name)) {
            return false;
        }
    }

    return true;
}


// ------------------------------------------------------------------------------------------------
//                                  GameEvent::PlayerEliminatedData
// ------------------------------------------------------------------------------------------------
std::size_t GameEvent::PlayerEliminatedData::serialize_binary(
        std::string &buf, std::size_t offset) const noexcept {
    // TODO
}


bool GameEvent::PlayerEliminatedData::deserialize_binary(
        const std::string &data, std::size_t offset, std::size_t size) noexcept {
    return false;  // TODO
}


bool GameEvent::PlayerEliminatedData::validate(GameEvent::Format fmt) const  noexcept {
    if (fmt == Format::Text) {
        if (player_name.empty() || !validate_player_name(player_name)) {
            return false;
        }
    }

    return true;
}
