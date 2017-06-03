#ifndef SIKTACKA_CLIENT_HPP
#define SIKTACKA_CLIENT_HPP


#include <common/network/HostAddress.hpp>


class Client {
private:
    HostAddress server_address;
    HostAddress gui_address;
    std::string player_name;

public:
    Client(int argc, char *argv[]) noexcept;

private:
    void parseArguments(int argc, char *argv[]) noexcept;
};


#endif //SIKTACKA_CLIENT_HPP
