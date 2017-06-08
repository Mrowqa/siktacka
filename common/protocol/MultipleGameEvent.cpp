#include <common/protocol/MultipleGameEvent.hpp>
#include <common/network/UdpSocket.hpp>

#include <cassert>
#include <algorithm>
#include <endian.h>


std::pair<std::string, uint32_t> MultipleGameEvent::prepare_packet_from_cache(
        const std::deque<std::string> &cache, uint32_t next_no) const noexcept {
    std::string buffer;
    buffer.reserve(max_datagram_size);
    buffer.resize(sizeof(uint32_t));
    *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(game_id);

    for (; next_no < cache.size()
           && buffer.size() + cache[next_no].size() <= max_datagram_size; next_no++) {
        buffer += cache[next_no];
    }

    return std::make_pair(buffer, next_no);
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
        auto result = ev_ptr->deserialize(GameEvent::Format::Binary,
                                          data.substr(offset, packet_size));
        offset += packet_size;

        if (result == GameEvent::DeserializationResult::Error) {
            break;
        }
        else if (result == GameEvent::DeserializationResult::UnknownEventType) {
            continue;
        }

        events.emplace_back(std::move(ev_ptr));
    }

    // all single events have been already validated
    return events.size() > 0;
}


bool MultipleGameEvent::validate() const noexcept {
    return std::all_of(events.begin(), events.end(), [](const auto &ev) {
        return ev != nullptr && ev->validate(GameEvent::Format::Binary);
    });
}
