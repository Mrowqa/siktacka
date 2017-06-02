#include "HostAddress.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstring>
#include <sstream>


static constexpr long long min_port = 0;
static constexpr long long max_port = 65535;


HostAddress::HostAddress(const std::string &address) {
    resolve(address);
}

const addrinfo *HostAddress::get() const noexcept {
    return address_ptr.get();
}

bool HostAddress::resolve(const std::string &address) {
    address_ptr.reset();

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

    address_ptr.reset(result);
    return true;
}

HostAddress::IpVersion HostAddress::get_ip_version() const noexcept {
    auto ptr = get();
    if (ptr == nullptr) {
        return IpVersion::None;
    }

    switch (ptr->ai_family) {
        case AF_INET:  return IpVersion::IPv4;
        case AF_INET6: return IpVersion::IPv6;
        default:       return IpVersion::None;
    }
}

void HostAddress::AddrInfoDeleter::operator()(addrinfo *ptr) const noexcept {
    freeaddrinfo(ptr);
}
