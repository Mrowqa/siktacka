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
        sockaddr addr;
        socklen_t addrlen;
        IpVersion ip_version;
    };

private:
    std::unique_ptr<SocketAddress> addr_ptr;

public:

    HostAddress() noexcept = default;
    HostAddress(const std::string &address);

    void set(const SocketAddress& sock_addr) noexcept;
    const SocketAddress *get() const noexcept;
    bool resolve(const std::string& address);
};


#endif //SIKTACKA_HOSTADDRESS_HPP
