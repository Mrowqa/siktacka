#pragma once

#include <common/network/Socket.hpp>


// TODO write comment!
class TcpSocket final : public Socket {
private:
    std::string recv_buffer;

public:
    TcpSocket() noexcept = default;

    Socket::Status init(HostAddress::IpVersion ip_ver) noexcept; // only for server before bind
    Socket::Status connect(const HostAddress &server_addr) noexcept;  // blocking operation

    // Note: sent argument must be initialized to number of already sent bytes
    //       and it will be updated by the function.
    Socket::Status send(const std::string &data, std::size_t &sent) noexcept;
    // buffer_size is maximum number of bytes to receive.
    // buffer will be resized to fit amount of received data.
    Socket::Status receive(std::string &buffer, std::size_t buffer_size) noexcept;

    // Note: You should use only send/receive or only send_line/receive_line!
    // Note: new line character is being added by send_line and is cut off by receive_line
    Socket::Status send_line(const std::string &data) noexcept;
    Socket::Status receive_line(std::string &buffer) noexcept;
};
