#include "HostAddress.hpp"

#include <cassert>
#include <cstring>
#include <sstream>


HostAddress::HostAddress(const std::string &address) {
    resolve(address);
}

const addrinfo &HostAddress::get() const noexcept {
    assert(address_ptr != nullptr);
    return *address_ptr;
}

void HostAddress::resolve(const std::string &address) {
    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = 0;          // Allow all types (i.a. UDP & TCP)
    hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
    hints.ai_protocol = 0;          // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    addrinfo *result = nullptr;
    auto divider_pos = address.rfind(':');
    auto hostname = address.substr(0, divider_pos);
    auto port = divider_pos < address.length() - 1 ? address.substr(divider_pos + 1) : "";
    int s = getaddrinfo(hostname.empty() ? nullptr : hostname.c_str(),
                        port.empty() ? nullptr : port.c_str(), &hints, &result);
    if (s != 0) {
        std::ostringstream msg;
        msg << "getaddrinfo: " << gai_strerror(s);
        throw std::runtime_error(msg.str());
    }

    address_ptr.reset(result);
}

void HostAddress::AddrInfoDeleter::operator()(addrinfo *ptr) const noexcept {
    freeaddrinfo(ptr);
}