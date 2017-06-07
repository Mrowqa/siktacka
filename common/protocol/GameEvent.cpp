#include <common/protocol/GameEvent.hpp>
#include <common/protocol/utils.hpp>
#include <common/network/UdpSocket.hpp>
#include <common/utils.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <endian.h>
#include <zlib.h>


// ------------------------------------------------------------------------------------------------
//                                         GameEvent
// ------------------------------------------------------------------------------------------------
static const std::size_t min_size_of_binary_packet =
        sizeof(uint32_t)     // len
        + sizeof(uint32_t)   // event_no
        + sizeof(uint8_t)    // type
        + sizeof(uint32_t);  // crc32


std::string GameEvent::serialize(GameEvent::Format fmt) const noexcept {
    assert(validate(fmt));

    return fmt == GameEvent::Format::Binary ? serialize_binary() : serialize_text();
}


GameEvent::DeserializationResult GameEvent::deserialize(GameEvent::Format fmt,
                                                        const std::string &data) noexcept {
    if (fmt != GameEvent::Format::Binary) {
        exit_with_error("GameEvent: deserialize does not support Format::Text");
    }

    if (data.size() < min_size_of_binary_packet || max_datagram_size < data.size()) {
        return DeserializationResult::Error;
    }

    const uint32_t len = be32toh(*reinterpret_cast<const uint32_t*>(&data[0]));
    if (data.size() != len + sizeof(uint32_t) * 2) {  // len + event_* + crc32
        return DeserializationResult::Error;
    }

    const std::size_t event_offset = 4;
    const uint32_t crc32_value = crc32(0, reinterpret_cast<const Bytef*>(&data[0]),
                                       event_offset + len);
    const uint32_t crc32_recvd = be32toh(*reinterpret_cast<const uint32_t*>(
                                         &data[event_offset + len]));
    if (crc32_value != crc32_recvd) {
        return DeserializationResult::Error;
    }

    event_no = be32toh(*reinterpret_cast<const uint32_t*>(&data[event_offset]));
    type = static_cast<Type>(
            *reinterpret_cast<const uint8_t*>(&data[event_offset + 4]));

    bool success = true;
    const std::size_t type_data_offset = sizeof(uint32_t) + sizeof(int8_t);
    switch (type) {
        case Type::NewGame:
            success = new_game_data.deserialize_binary(
                    data, event_offset + type_data_offset, len - type_data_offset);
            break;

        case Type::Pixel:
            success = pixel_data.deserialize_binary(
                    data, event_offset + type_data_offset, len - type_data_offset);
            break;

        case Type::PlayerEliminated:
            success = player_eliminated_data.deserialize_binary(
                    data, event_offset + type_data_offset, len - type_data_offset);
            break;

        case Type::GameOver:
            break;

        default:
            return DeserializationResult::UnknownEventType;
            break;
    }

    return success && validate(Format::Binary)
           ? DeserializationResult::Success : DeserializationResult::Error;
}


bool GameEvent::validate(GameEvent::Format fmt) const noexcept {
    switch (type) {
        case Type::NewGame:  return new_game_data.validate();
        case Type::Pixel:    return pixel_data.validate(fmt);
        case Type::PlayerEliminated:
                             return player_eliminated_data.validate(fmt);
        case Type::GameOver: return fmt == Format::Binary;
        default:             return false;
    }
}


std::string GameEvent::serialize_binary() const noexcept {
    std::string buffer(max_datagram_size, '\0');

    uint32_t len = sizeof(event_no) + sizeof(type);
    const std::size_t event_offset = sizeof(len);
    const std::size_t data_field_offset = sizeof(len) + len;

    *reinterpret_cast<uint32_t*>(&buffer[event_offset]) = htobe32(event_no);
    *reinterpret_cast<uint8_t*> (&buffer[event_offset + 4]) = static_cast<uint8_t>(type);
    switch (type) {
        case Type::NewGame:
            len += new_game_data.serialize_binary(buffer, data_field_offset);
            break;

        case Type::Pixel:
            len += pixel_data.serialize_binary(buffer, data_field_offset);
            break;

        case Type::PlayerEliminated:
            len += player_eliminated_data.serialize_binary(buffer, data_field_offset);
            break;

        case Type::GameOver:
            break;
    }

    *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(len);
    const uint32_t crc32_value = crc32(0, reinterpret_cast<const Bytef*>(&buffer[0]),
                                       event_offset + len);
    *reinterpret_cast<uint32_t*>(&buffer[event_offset + len]) = htobe32(crc32_value);

    buffer.resize(event_offset + len + sizeof(crc32_value));

    return buffer;
}


std::string GameEvent::serialize_text() const noexcept {
    assert(type == Type::NewGame || type == Type::Pixel || type == Type::PlayerEliminated);

    std::ostringstream str;

    switch (type) {
        case Type::NewGame:
            str << "NEW_GAME " << new_game_data.maxx << ' ' << new_game_data.maxy;
            for (const auto &name : new_game_data.players_names) {
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
                sizeof(uint32_t)                 // game_id
                + min_size_of_binary_packet  // len, event_no, type, crc32
                + sizeof(uint32_t) * 2           // max{x,y}
);

std::size_t GameEvent::NewGameData::calculate_used_names_capacity() const noexcept {
    std::size_t result = 0;

    for (const auto &name : players_names) {
        result += name.size() + 1;
    }

    return result;
}


std::size_t GameEvent::NewGameData::serialize_binary(std::string &buf, std::size_t offset) const noexcept {
    char *const buf_ptr = &buf[offset];
    *reinterpret_cast<uint32_t*>(buf_ptr) = htobe32(maxx);
    *reinterpret_cast<uint32_t*>(buf_ptr + 4) = htobe32(maxy);

    auto it = buf_ptr + 8;
    for (const auto &name : players_names) {
        memcpy(it, name.data(), name.size());
        it += name.size();
        *it++ = '\0';
    }

    return it - buf_ptr;
}


bool GameEvent::NewGameData::deserialize_binary(const std::string &data, std::size_t offset,
                                                std::size_t size) noexcept {
    assert(offset + size <= data.length());
    if (size < 8) {
        return false;
    }

    const char *const data_ptr = &data[offset];
    maxx = be32toh(*reinterpret_cast<const uint32_t*>(data_ptr));
    maxy = be32toh(*reinterpret_cast<const uint32_t*>(data_ptr + 4));

    players_names.clear();
    auto it = data_ptr + 8;
    auto end_it = it;
    auto const data_end = &data[offset + size];

    while (it < data_end) {
        while (end_it < data_end && *end_it) {
            end_it++;
        }

        if (end_it == data_end) {
            return false;
        }

        players_names.emplace_back(it, end_it - it);
        it = end_it = end_it + 1;
    }

    return validate();
}


bool GameEvent::NewGameData::validate() const noexcept {
    if (calculate_used_names_capacity() > names_capacity) {
        return false;
    }

    if (players_names.size() < 2) {
        return false;
    }

    return std::all_of(players_names.begin(), players_names.end(), [](const auto &name) {
        return !name.empty() && validate_player_name(name);
    });
}


// ------------------------------------------------------------------------------------------------
//                                     GameEvent::PixelData
// ------------------------------------------------------------------------------------------------
std::size_t GameEvent::PixelData::serialize_binary(std::string &buf, std::size_t offset) const noexcept {
    char *const buf_ptr = &buf[offset];
    *reinterpret_cast<uint8_t*>(buf_ptr) = player_no;
    *reinterpret_cast<uint32_t*>(buf_ptr + 1) = htobe32(x);
    *reinterpret_cast<uint32_t*>(buf_ptr + 5) = htobe32(y);

    return 9;
}


bool GameEvent::PixelData::deserialize_binary(const std::string &data, std::size_t offset,
                                              std::size_t size) noexcept {
    assert(offset + size <= data.length());
    if (size != 9) {
        return false;
    }

    const char *const data_ptr = &data[offset];
    player_no = *reinterpret_cast<const uint8_t*>(data_ptr);
    x = be32toh(*reinterpret_cast<const uint32_t*>(data_ptr + 1));
    y = be32toh(*reinterpret_cast<const uint32_t*>(data_ptr + 5));

    return true;
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
    *reinterpret_cast<uint8_t*>(&buf[offset]) = player_no;

    return 1;
}


bool GameEvent::PlayerEliminatedData::deserialize_binary(
        const std::string &data, std::size_t offset, std::size_t size) noexcept {
    assert(offset + size <= data.length());
    if (size != 1) {
        return false;
    }

    player_no = *reinterpret_cast<const uint8_t*>(&data[offset]);

    return true;
}


bool GameEvent::PlayerEliminatedData::validate(GameEvent::Format fmt) const  noexcept {
    if (fmt == Format::Text) {
        if (player_name.empty() || !validate_player_name(player_name)) {
            return false;
        }
    }

    return true;
}
