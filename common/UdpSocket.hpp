#ifndef SIKTACKA_UDPSOCKET_HPP
#define SIKTACKA_UDPSOCKET_HPP


#include "Socket.hpp"


extern const std::size_t max_datagram_size;


class UdpSocket final : Socket {
private:
    HostAddress::SocketAddress preallocated_sock_addr;

public:
    UdpSocket() noexcept = default;

    // TODO gdzie noexcept?
    Socket::Status init(HostAddress::IpVersion ip_ver) noexcept;
    Socket::Status send(const std::string& data, const HostAddress& dst_addr) noexcept;
    Socket::Status receive(std::string& buffer, HostAddress& src_addr) noexcept;
};


#endif //SIKTACKA_UDPSOCKET_HPP
