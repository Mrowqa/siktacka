#include "Socket.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>
#include <unistd.h>


Socket::~Socket() noexcept {
    if (sockfd != -1) {
        close(sockfd);
    }
}


void Socket::set_blocking(bool block) noexcept {
    int status = fcntl(sockfd, F_GETFL);
    int new_status = block ? status & ~O_NONBLOCK : status | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, new_status) == -1) {
        std::ostringstream msg;
        msg << "Failed to change blocking mode of socket: " << strerror(errno);
        throw std::runtime_error(msg.str());
    }
}


bool Socket::is_blocking() const noexcept {
    return fcntl(sockfd, F_GETFL) & O_NONBLOCK;
}
