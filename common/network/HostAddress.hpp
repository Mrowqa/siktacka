#ifndef SIKTACKA_HOSTADDRESS_HPP
#define SIKTACKA_HOSTADDRESS_HPP


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <memory>
#include <string>


class HostAddress final {
public:
    enum class IpVersion {
        None,
        IPv4,
        IPv6,
    };

    struct SocketAddress {
        union {
            sockaddr addr;
            sockaddr_in addr_v4;
            sockaddr_in6 addr_v6;
        };
        socklen_t addrlen;
        IpVersion ip_version;

        SocketAddress() noexcept;
        void clear() noexcept;
    };

private:
    std::unique_ptr<SocketAddress> addr_ptr;

public:

    HostAddress() noexcept = default;
    HostAddress(const std::string &host, unsigned short port);

    void set(const SocketAddress &sock_addr) noexcept;
    const SocketAddress *get() const noexcept;
    bool resolve(const std::string &host, unsigned short port);
    std::string to_string() const noexcept;
};


#endif //SIKTACKA_HOSTADDRESS_HPP
