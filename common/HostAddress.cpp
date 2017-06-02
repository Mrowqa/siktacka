#include "HostAddress.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstring>


static constexpr long long min_port = 0;
static constexpr long long max_port = 65535;


HostAddress::HostAddress(const std::string &address) {
    resolve(address);
}


void HostAddress::set(const SocketAddress& sock_addr) noexcept {
    if (addr_ptr == nullptr) {
        addr_ptr = std::make_unique<SocketAddress>(sock_addr);
    }
    else {
        memcpy(addr_ptr.get(), &sock_addr, sizeof(SocketAddress));
    }
}


const HostAddress::SocketAddress *HostAddress::get() const noexcept {
    return addr_ptr.get();
}


bool HostAddress::resolve(const std::string &address) {
    addr_ptr.reset();

    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = 0;          // Allow all types (i.a. UDP & TCP)
    hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
    hints.ai_protocol = 0;          // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    auto divider_pos = address.rfind(':');
    auto hostname = address.substr(0, divider_pos);
    auto port = divider_pos < address.length() - 1 ? address.substr(divider_pos + 1) : "";

    if (!port.empty()) {
        auto port_num = from_string<long long>(port);
        if (port_num < min_port || max_port < port_num) {
            return false;
        }
    }

    addrinfo *result = nullptr;
    int s = getaddrinfo(hostname.empty() ? nullptr : hostname.c_str(),
                        port.empty() ? nullptr : port.c_str(), &hints, &result);
    if (s != 0) {
        return false;
    }

    assert(result != nullptr);
    auto ip_ver = result->ai_family == AF_INET ? IpVersion::IPv4 :
                  result->ai_family == AF_INET6 ? IpVersion::IPv6 : IpVersion::None;
    SocketAddress sock_addr = {*result->ai_addr, result->ai_addrlen, ip_ver};
    addr_ptr = std::make_unique<SocketAddress>(sock_addr);
    freeaddrinfo(result);
    return true;
}
