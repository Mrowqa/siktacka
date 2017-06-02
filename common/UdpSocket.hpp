#ifndef SIKTACKA_UDPSOCKET_HPP
#define SIKTACKA_UDPSOCKET_HPP


#include "Socket.hpp"


extern const std::size_t max_datagram_size;


class UdpSocket final : Socket {
public:
    UdpSocket() noexcept = default;

    // TODO gdzie noexcept?
    Socket::Status init(HostAddress::IpVersion version) noexcept;
    Socket::Status send(const std::string& data, std::size_t& sent, const HostAddress& dst_addr);
    Socket::Status receive(std::string& data, HostAddress& src_addr);
};


#endif //SIKTACKA_UDPSOCKET_HPP
