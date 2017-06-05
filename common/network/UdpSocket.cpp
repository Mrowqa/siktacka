#include <common/network/UdpSocket.hpp>

#include <cassert>


constexpr std::size_t max_datagram_size = 512;


Socket::Status UdpSocket::init(HostAddress::IpVersion ip_ver) noexcept {
    return Socket::init(ip_ver, SOCK_DGRAM);
}


Socket::Status UdpSocket::send(const std::string &data, const HostAddress &dst_addr) noexcept {
    auto addr_ptr = dst_addr.get();
    assert(addr_ptr != nullptr);

    if (data.size() > max_datagram_size) {
        return Status::Error;
    }

    int sent = sendto(sockfd, data.c_str(), data.size(), 0,
                      &addr_ptr->addr, addr_ptr->addrlen);

    if (sent < 0) {
        return get_error_status();
    }

    return Status::Done;
}


Socket::Status UdpSocket::receive(std::string &buffer, HostAddress &src_addr) noexcept {
    buffer.resize(max_datagram_size);
    preallocated_sock_addr.clear();
    preallocated_sock_addr.ip_version = ip_ver;

    int bytes_received = recvfrom(sockfd, &buffer[0], max_datagram_size, 0,
                                  &preallocated_sock_addr.addr,
                                  &preallocated_sock_addr.addrlen);

    if (bytes_received < 0) {
        return get_error_status();
    }

    buffer.resize(bytes_received);
    src_addr.set(preallocated_sock_addr);

    return Status::Done;
}
