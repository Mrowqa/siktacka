#include <common/protocol/MultipleGameEvent.hpp>
#include <common/network/UdpSocket.hpp>

#include <cassert>
#include <algorithm>
#include <endian.h>


MultipleGameEvent::SerializedContainer MultipleGameEvent::serialize() const noexcept {
    assert(validate());

    SerializedContainer result;
    std::string buffer;

    auto add_header = [](std::string &buffer, uint32_t game_id) {
        buffer.resize(sizeof(uint32_t));
        *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(game_id);
    };

    add_header(buffer, game_id);
    for (const auto &ev : events) {
        std::string serialized_ev = ev->serialize(GameEvent::Format::Binary);
        if (buffer.size() + serialized_ev.size() > max_datagram_size) {
            result.emplace_back(std::move(buffer));
            add_header(buffer, game_id);
        }

        buffer += serialized_ev; // size is proper (we assume user has called validate())
    }

    if (buffer.size() > sizeof(uint32_t)) {
        result.emplace_back(std::move(buffer));
    }

    return result;
}


bool MultipleGameEvent::deserialize(const std::string &data) noexcept {
    events.clear();

    if (data.size() < sizeof(uint32_t)) {
        return false;
    }

    game_id = be32toh(*reinterpret_cast<const uint32_t*>(&data[0]));
    std::size_t offset = sizeof(uint32_t);

    while (offset < data.size()) {
        uint32_t len = be32toh(*reinterpret_cast<const uint32_t*>(&data[offset]));
        auto packet_size = len + sizeof(uint32_t) * 2; // +sizeof(len) +sizeof(crc32)

        if (offset + packet_size > data.size()) {
            return false;
        }

        auto ev_ptr = std::make_unique<GameEvent>();
        if (!ev_ptr->deserialize(GameEvent::Format::Binary, data.substr(offset, packet_size))) {
            return false;
        }

        offset += packet_size;
    }

    // all single events have been already validated
    return events.size() > 0;
}


// TODO check rest of code if can be simplified with <algorithm>
bool MultipleGameEvent::validate() const noexcept {
    return std::all_of(events.begin(), events.end(), [](const auto &ev) {
        return ev != nullptr && ev->validate(GameEvent::Format::Binary);
    });
}
