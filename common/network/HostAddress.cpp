#include <common/network/HostAddress.hpp>
#include <common/utils.hpp>

#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <cstring>


const uint16_t HostAddress::min_port = 0;
const uint16_t HostAddress::max_port = 65535;


HostAddress::HostAddress(const std::string &host, unsigned short port) {
    resolve(host, port);
}


HostAddress::HostAddress(const HostAddress &addr) {
    auto sock_addr = addr.get();
    if (sock_addr) {
        set(*sock_addr);
    }
}


void HostAddress::set(const SocketAddress &sock_addr) noexcept {
    if (addr_ptr == nullptr) {
        addr_ptr = std::make_unique<SocketAddress>();
    }
    memcpy(addr_ptr.get(), &sock_addr, sizeof(SocketAddress));
}


const HostAddress::SocketAddress *HostAddress::get() const noexcept {
    return addr_ptr.get();
}


bool HostAddress::resolve(const std::string &host, unsigned short port) {
    addr_ptr.reset();
    addrinfo *result = nullptr;
    int s = getaddrinfo(host.empty() ? nullptr : host.c_str(),
                        nullptr, nullptr, &result);
    if (s != 0) {
        return false;
    }

    assert(result != nullptr);

    bool success = true;
    auto ip_ver = result->ai_family == AF_INET ? IpVersion::IPv4 :
                  result->ai_family == AF_INET6 ? IpVersion::IPv6 : IpVersion::None;
    if (ip_ver == IpVersion::None) {
        success = false;
    }
    else {
        // Had to set it manually, cause we can not fully trust getaddrinfo.
        // TODO refactor to fully trust getaddrinfo, again (second arg & result)
        addr_ptr = std::make_unique<SocketAddress>();
        addr_ptr->addrlen = result->ai_addrlen;
        addr_ptr->ip_version = ip_ver;

        if (ip_ver == IpVersion::IPv4) {
            addr_ptr->addr_v4.sin_family = AF_INET;
            addr_ptr->addr_v4.sin_addr =
                    reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr;
            addr_ptr->addr_v4.sin_port = htons(port);
        }
        else {  // IPv6
            addr_ptr->addr_v6.sin6_family = AF_INET6;
            memcpy(&addr_ptr->addr_v6.sin6_addr,
                   &reinterpret_cast<sockaddr_in6*>(result->ai_addr)->sin6_addr,
                    sizeof(in6_addr));
            addr_ptr->addr_v6.sin6_port = htons(port);
        }
    }

    freeaddrinfo(result);
    return success;
}


std::string HostAddress::to_string() const noexcept {
    assert(addr_ptr != nullptr);
    assert(addr_ptr->ip_version != IpVersion::None);

    std::string result;
    constexpr std::size_t max_ip_text_length = 64;  // in fact 39, but it does not hurt to give more space
    result.resize(max_ip_text_length);

    if (addr_ptr->ip_version == IpVersion::IPv4) {
        auto sockaddr_ptr = reinterpret_cast<sockaddr_in*>(&addr_ptr->addr);
        if (inet_ntop(AF_INET, &sockaddr_ptr->sin_addr, &result[0], max_ip_text_length) == nullptr) {
            return "";
        }
        result.resize(result.find('\0'));
        result += ":";
        result += std::to_string(ntohs(sockaddr_ptr->sin_port));
        return result;
    }
    else {  // IPv6
        result[0] = '[';
        auto sockaddr6_ptr = reinterpret_cast<sockaddr_in6*>(&addr_ptr->addr);
        if (inet_ntop(AF_INET6, &sockaddr6_ptr->sin6_addr, &result[1], max_ip_text_length) == nullptr) {
            return "";
        }
        result.resize(result.find('\0'));
        result += "]:";
        result += std::to_string(ntohs(sockaddr6_ptr->sin6_port));
        return result;
    }
}


bool HostAddress::operator==(const HostAddress &rhs) const noexcept {
    return compare(rhs) == 0;
}


bool HostAddress::operator!=(const HostAddress &rhs) const noexcept {
    return compare(rhs) != 0;
}


bool HostAddress::operator<(const HostAddress &rhs) const noexcept {
    return compare(rhs) < 0;
}


int HostAddress::compare(const HostAddress &rhs) const noexcept {
    if (addr_ptr == nullptr) {
        return rhs.addr_ptr == nullptr ? 0 : -1;
    }

    if (rhs.addr_ptr == nullptr) {
        return 1;
    }

    if (addr_ptr->ip_version != rhs.addr_ptr->ip_version) {
        using UndType = std::underlying_type<IpVersion>::type;
        return static_cast<UndType>(addr_ptr->ip_version) <
               static_cast<UndType>(rhs.addr_ptr->ip_version) ? -1 : 1;
    }

    switch (addr_ptr->ip_version) {
        case IpVersion::IPv4:
            return memcmp(&addr_ptr->addr_v4, &rhs.addr_ptr->addr_v4, sizeof(addr_ptr->addr_v4));
            break;

        case IpVersion::IPv6:
            return memcmp(&addr_ptr->addr_v6, &rhs.addr_ptr->addr_v6, sizeof(addr_ptr->addr_v6));
            break;

        case IpVersion::None:
        default:
            return 0;
            break;
    }
}


HostAddress::SocketAddress::SocketAddress() noexcept {
    clear();
}

void HostAddress::SocketAddress::clear() noexcept {
    addrlen = std::max(sizeof(addr_v4), sizeof(addr_v6));
    memset(&addr, 0, addrlen);
    ip_version = IpVersion::None;
}
