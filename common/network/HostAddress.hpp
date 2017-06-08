#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <memory>
#include <string>


// Class representing full address (IP & port number).
// It can also resolve name to IP.
class HostAddress final {
public:
    enum class IpVersion {
        None,
        IPv4,
        IPv6,
    };

    struct SocketAddress final {
        union {
            sockaddr addr;
            sockaddr_in addr_v4;
            sockaddr_in6 addr_v6;
        }; // because they have different sizes
        socklen_t addrlen;
        IpVersion ip_version;

        SocketAddress() noexcept;
        // fills addr_* (union) with null bytes, sets addrlen to max possible size
        // and sets ip_version to None.
        void clear() noexcept;
    };

    static const uint16_t min_port;
    static const uint16_t max_port;

private:
    std::unique_ptr<SocketAddress> addr_ptr;

public:
    HostAddress() noexcept = default;
    HostAddress(const std::string &host, unsigned short port);
    HostAddress(HostAddress &&addr) = default;
    HostAddress(const HostAddress &addr);

    // sets new socket address
    void set(const SocketAddress &sock_addr) noexcept;
    // returns pointer to SocketAddress if present, nullptr otherwise
    const SocketAddress *get() const noexcept;
    // resolves hostname and port to SocketAddress and returns true if succeeded
    // sets nullptr on fail
    bool resolve(const std::string &host, unsigned short port);
    // returns string representation of address
    // must not be called when get() == nullptr
    std::string to_string() const noexcept;

    bool operator==(const HostAddress &rhs) const noexcept;
    bool operator!=(const HostAddress &rhs) const noexcept;
    bool operator< (const HostAddress &rhs) const noexcept;

private:
    int compare(const HostAddress &rhs) const noexcept;
};
