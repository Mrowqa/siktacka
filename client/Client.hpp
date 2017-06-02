#ifndef SIKTACKA_CLIENT_HPP
#define SIKTACKA_CLIENT_HPP


#include "../common/HostAddress.hpp"


class Client {
private:
    std::unique_ptr<HostAddress> server_address;
    std::unique_ptr<HostAddress> gui_address;
    std::string player_name;

public:
    Client(int argc, char *argv[]) noexcept;

private:
    void parseArguments(int argc, char *argv[]) noexcept;
};


#endif //SIKTACKA_CLIENT_HPP
