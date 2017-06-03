#include "TcpSocket.hpp"


#include <cassert>


Socket::Status TcpSocket::init(HostAddress::IpVersion ip_ver) noexcept {
    return Socket::init(ip_ver, SOCK_DGRAM);
}


Socket::Status TcpSocket::connect(const HostAddress &server_addr) noexcept {
    auto addr_ptr = server_addr.get();
    assert(addr_ptr != nullptr);

    auto status = init(addr_ptr->ip_version);
    if (status != Status::Done) {
        return status;
    }

    bool blocking_status = is_blocking();
    if (!blocking_status) {
        set_blocking(true);
    }

    int s = ::connect(sockfd, &addr_ptr->addr, addr_ptr->addrlen);
    if (!blocking_status) {
        set_blocking(false);
    }

    return s >= 0 ? Status::Done : get_error_status();
}


Socket::Status TcpSocket::send(const std::string &data, std::size_t& sent, std::size_t offset) noexcept {
    if (data.size() <= offset) {
        return Status::Error;
    }

    int result = 0;
    for (sent = offset; sent < data.size(); sent += result) {
        result = ::send(sockfd, data.c_str() + sent, data.size() - sent, 0);

        if (result < 0) {
            Status status = get_error_status();

            return status == Status::NotReady && sent > offset
                   ? Status::Partial : status;
        }
    }

    return Status::Done;
}


Socket::Status TcpSocket::receive(std::string &buffer, std::size_t buffer_size) noexcept {
    assert(buffer_size > 0);
    buffer.resize(buffer_size);

    auto received = recv(sockfd, &buffer[0], buffer_size, 0);

    if (received > 0) {
        buffer.resize(received);
        return Status::Done;
    } else if (received == 0) {
        return Status::Disconnected;
    } else {
        return get_error_status();
    }
}

Socket::Status TcpSocket::send_line(const std::string &data) noexcept {
    std::size_t sent = 0;
    Socket::Status status;

    std::string to_sent = data + "\n";
    do {
        status = send(to_sent, sent, sent);
    } while (status == Status::Partial);

    return status;
}

Socket::Status TcpSocket::receive_line(std::string &buffer) noexcept {
    auto status = Status::Done;
    bool waiting_for_rest_of_command = false;
    const std::size_t BUFF_SIZE = 256;
    std::string chunk_buffer;

    buffer = recv_buffer;
    recv_buffer.clear();
    for (size_t scanned_bytes_cnt = 0;
         buffer.find('\n', scanned_bytes_cnt) == std::string::npos; ) {
        scanned_bytes_cnt = buffer.size();
        status = receive(chunk_buffer, BUFF_SIZE);

        if (status == Status::NotReady) {
            if (!waiting_for_rest_of_command) {
                break;
            }
        }
        else if (status != Status::Done) {
            break;
        }

        buffer += chunk_buffer;
        waiting_for_rest_of_command = true;
    }

    if (status == Status::Done) {
        size_t end_offset = buffer.find('\n');
        recv_buffer = buffer.substr(end_offset + 1);
        buffer.resize(end_offset);
    }
    else if (waiting_for_rest_of_command) {
        recv_buffer = buffer;
    }

    return status;
}
