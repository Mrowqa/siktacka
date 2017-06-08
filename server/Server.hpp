#pragma once

#include <common/RandomNumberGenerator.hpp>
#include <common/network/UdpSocket.hpp>
#include <common/protocol/HeartBeat.hpp>

#include <chrono>
#include <deque>
#include <list>
#include <map>
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
        HostAddress address;
        uint64_t session_id;
        std::string name;

        int8_t player_no;  // number of player during game, -1 if observer
        bool watching_game;
        std::chrono::system_clock::time_point last_heartbeat_time;
        bool ready_to_play;
        uint32_t next_event_no;
    };

    // for fast and balanced iterating through all clients and lookups
public:  using ClientContainer = std::map<HostAddress, ClientSession>;


    // server config
    struct {
        uint32_t map_width = 800;
        uint32_t map_height = 600;
        uint32_t rounds_per_second = 50;
        uint32_t turning_speed = 6;
        uint16_t port_number = 12345;
    } config;

    // game state
    struct {
        using system_clock = std::chrono::system_clock;

        std::vector<Player> players;
        uint32_t game_id = 0;
        bool game_in_progress = false;
        std::vector<bool> map;
        std::deque<std::string> serialized_events; // to think about TODO
        system_clock::time_point next_update_time = system_clock::now();
    } game_state;

    // server state
    struct {
        RandomNumberGenerator rand_gen;
        ClientContainer clients;
        ClientContainer::iterator next_client;  // who will get game events updates
                                                // if waiting for any
    } server_state;


public:
    Server(int argc, char *argv[]);
    void run();

private:
    void parse_arguments(int argc, char *argv[]);
    void print_usage(const char *name) const noexcept;
    void init_server();
    void check_clients_connections();
    ClientContainer::iterator disconnect_client(ClientContainer::iterator client);
    void handle_clients_input();
    ClientContainer::iterator handle_client_session(
            const HostAddress &client_addr, const HeartBeat &hb);
    bool check_name_availability(const std::string &name) const noexcept;
    void send_events_to_clients();
    void update_game_state();
    bool game_update_pending() const;
    bool pending_work() const;

    // start_new_game ...
};
