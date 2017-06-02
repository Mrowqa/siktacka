#include "UdpSocket.hpp"


constexpr std::size_t max_datagram_size = 512;


Socket::Status UdpSocket::init(HostAddress::IpVersion version) noexcept {
    return Socket::init(version, SOCK_DGRAM);
}


Socket::Status UdpSocket::send(const std::string &data, std::size_t &sent, const HostAddress &dst_addr) {
    return Status::Done;
    // TODO implement
}


Socket::Status UdpSocket::receive(std::string &data, HostAddress &src_addr) {
    return Status::Done;
    // TODO implement
}
