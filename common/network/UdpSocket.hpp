#pragma once

#include <common/network/Socket.hpp>


extern const std::size_t max_datagram_size;


// TODO write comment!
class UdpSocket final : public Socket {
private:
    HostAddress::SocketAddress preallocated_sock_addr;

public:
    UdpSocket() noexcept = default;

    Socket::Status init(HostAddress::IpVersion ip_ver) noexcept;
    Socket::Status send(const std::string &data, const HostAddress &dst_addr) noexcept;
    // buffer will be resized to fit amount of received data.
    Socket::Status receive(std::string &buffer, HostAddress &src_addr) noexcept;
};
