#pragma once

#include <common/network/HostAddress.hpp>


class Client final {
private:
    HostAddress server_address;
    HostAddress gui_address;
    std::string player_name;

public:
    Client(int argc, char *argv[]) noexcept;
    void run() noexcept;

private:
    void parse_arguments(int argc, char *argv[]) noexcept;
};
