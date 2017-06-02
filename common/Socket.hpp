#ifndef SIKTACKA_SOCKET_HPP
#define SIKTACKA_SOCKET_HPP


class Socket {
private:
    int sockfd = -1;

public:
    virtual ~Socket() noexcept;

    void set_blocking(bool block) noexcept;
    bool is_blocking() const noexcept;

protected:
    Socket() = default;
};


#endif //SIKTACKA_SOCKET_HPP
