#pragma once

#include <common/network/UdpSocket.hpp>
#include <common/RandomNumberGenerator.hpp>

#include <deque>
#include <vector>


class Server {
private:
    // socket
    UdpSocket socket;

    // represents player in game
    struct Player {
        std::string name;
        uint8_t turn_direction;
        bool alive;
        double pos_x;
        double pos_y;
        double angle;
    };

    // represents session of connected client
    struct ClientSession {

    };

    // server config
    struct {
        uint32_t map_width = 800;
        uint32_t map_height = 600;
        uint32_t rounds_per_second = 50;
        uint32_t turning_speed = 6;
        uint16_t port_number = 12345;
    } config;

    struct {
        std::vector<Player> players;
        uint32_t game_id = 0;
        bool game_in_progress = false;
        std::vector<bool> map;
        std::deque<std::string> serialized_events; // to think about TODO
    } game_state;

    struct {
        RandomNumberGenerator rand_gen;
        std::deque<ClientSession> clients;
    } server_state;


public:
    Server(int argc, char *argv[]);
    void run();

private:
    void parse_arguments(int argc, char *argv[]);
    void print_usage(const char *name) const noexcept;
    void init_server();
};
