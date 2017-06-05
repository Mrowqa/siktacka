#pragma once

#include <common/network/HostAddress.hpp>


// Base class for sockets.
class Socket {
protected:
    int sockfd = -1;
    HostAddress::IpVersion ip_ver = HostAddress::IpVersion::None;

public:
    enum class Status {
        Done,
        NotReady,
        Partial,  // only when partial data have been sent
        Disconnected,
        Error,
    };

    virtual ~Socket() noexcept;

    Socket::Status set_blocking(bool block) const noexcept;
    bool is_blocking() const noexcept;
    // must not be called before init() in derived classes
    Socket::Status bind(const HostAddress& host_addr) const noexcept;

protected:
    Socket() = default;
    Socket::Status get_error_status() const noexcept;
    Socket::Status init(HostAddress::IpVersion ip_ver, int socktype) noexcept;
};
