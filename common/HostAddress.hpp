#ifndef SIKTACKA_HOSTADDRESS_HPP
#define SIKTACKA_HOSTADDRESS_HPP


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <memory>
#include <string>


class HostAddress final {
private:
    class AddrInfoDeleter final {
    public:
        void operator()(addrinfo *ptr) const noexcept;
    };

    std::unique_ptr<addrinfo, AddrInfoDeleter> address_ptr;

public:
    enum class IpVersion {
        None,
        IPv4,
        IPv6,
    };

    HostAddress() noexcept = default;
    HostAddress(const std::string &address);

    const addrinfo *get() const noexcept;
    IpVersion get_ip_version() const noexcept;
    bool resolve(const std::string& address);
};


#endif //SIKTACKA_HOSTADDRESS_HPP
