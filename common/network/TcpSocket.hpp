#ifndef SIKTACKA_TCPSOCKET_HPP
#define SIKTACKA_TCPSOCKET_HPP


#include <common/network/Socket.hpp>


class TcpSocket final : Socket {
private:
    std::string recv_buffer;

public:
    TcpSocket() noexcept = default;

    Socket::Status init(HostAddress::IpVersion ip_ver) noexcept; // only for server before bind
    Socket::Status connect(const HostAddress &server_addr) noexcept;  // blocking operation
    Socket::Status send(const std::string &data, std::size_t &sent, std::size_t offset = 0) noexcept;
    Socket::Status receive(std::string &buffer, std::size_t buffer_size) noexcept;
    // You should use only send/receive or only send_line/receive_line!
    Socket::Status send_line(const std::string &data) noexcept;
    Socket::Status receive_line(std::string &buffer) noexcept;
};


#endif //SIKTACKA_TCPSOCKET_HPP
