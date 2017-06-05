#include <common/network/Socket.hpp>

#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>


Socket::~Socket() noexcept {
    if (sockfd >= 0) {
        close(sockfd);
    }
}


Socket::Status Socket::set_blocking(bool block) const noexcept {
    int status = fcntl(sockfd, F_GETFL);
    int new_status = block ? status & ~O_NONBLOCK : status | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, new_status) == -1) {
        return Status::Error;
    }

    return Status::Done;
}


bool Socket::is_blocking() const noexcept {
    return fcntl(sockfd, F_GETFL) & O_NONBLOCK;
}


Socket::Status Socket::get_error_status() const noexcept {
    // Sometimes the same as EAGAIN, and we can not have
    // switch with multiple cases for the same number.
    if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
        return Status::NotReady;
    }

    switch (errno) {
        case EAGAIN:
            return Status::NotReady;

        case ECONNABORTED:
        case ECONNRESET:
        case ETIMEDOUT:
        case ENETRESET:
        case ENOTCONN:
        case EPIPE:
            return Status::Disconnected;

        default:
            return Status::Error;
    }
}


Socket::Status Socket::bind(const HostAddress &host_addr) const noexcept {
    assert(sockfd >= 0);

    auto addr_ptr = host_addr.get();
    return ::bind(sockfd, &addr_ptr->addr, addr_ptr->addrlen) == 0
           ? Status::Done : Status::Error;
}


Socket::Status Socket::init(HostAddress::IpVersion ip_ver, int socktype) noexcept {
    assert(ip_ver != HostAddress::IpVersion::None);
    assert(sockfd == -1);

    sockfd = socket(ip_ver == HostAddress::IpVersion::IPv4 ? AF_INET : AF_INET6,
                    socktype, 0);

    if (sockfd < 0) {
        return Status::Error;
    }

    this->ip_ver = ip_ver;
    return Status::Done;
}
