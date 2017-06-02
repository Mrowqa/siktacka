#ifndef SIKTACKA_SOCKET_HPP
#define SIKTACKA_SOCKET_HPP


#include "HostAddress.hpp"


class Socket {
protected:
    int sockfd = -1;

public:
    enum class Status {
        Done,
        NotReady,
        Partial,
        Disconnected,
        Error,
    };

    virtual ~Socket() noexcept;

    Socket::Status set_blocking(bool block) const noexcept;
    bool is_blocking() const noexcept;
    Socket::Status bind(const HostAddress& host_addr) const noexcept;

protected:
    Socket() = default;
    Socket::Status get_error_status() const noexcept;
    Socket::Status init(HostAddress::IpVersion version, int socktype) noexcept;
};


#endif //SIKTACKA_SOCKET_HPP
