#pragma once

#include <common/RandomNumberGenerator.hpp>
#include <common/network/UdpSocket.hpp>
#include <common/protocol/GameEvent.hpp>
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
        int8_t turn_direction;
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

        int8_t player_no;  // number of players during game, -1 if observer
        bool watching_game;
        bool got_new_game_event;
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
        uint8_t alive_players_cnt = 0;
        uint32_t game_id = 0;
        bool game_in_progress = false;
        std::vector<bool> map;
        std::deque<std::string> serialized_events;
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
    // TODO separate game engine and clients management.
    // Clients management
    void parse_arguments(int argc, char *argv[]);
    void print_usage(const char *name) const noexcept;
    void init_server();
    void check_clients_connections();
    // Takes care of server_state.next_client
    void disconnect_client(ClientContainer::iterator client);
    void handle_clients_input();
    ClientContainer::iterator handle_client_session(
            const HostAddress &client_addr, const HeartBeat &hb);
    bool check_name_availability(const std::string &name) const noexcept;
    void send_events_to_clients();
    bool pending_work() const;

    // Game logic
    void update_game_state();
    void update_lasting_game_state();
    void start_new_game_if_possible();
    // Modifies event_no field, then serializes and caches
    // event in game_state.serialized_events
    void emit_game_event(GameEvent &event);
    bool is_on_map(double x, double y) const;
    void map_set(uint32_t x, uint32_t y);
    bool map_get(uint32_t x, uint32_t y) const;
    bool game_update_pending() const;
    void handle_pixel_event(uint8_t player_no, uint32_t x, uint32_t y);
    // Returns true if game is over.
    bool handle_player_eliminated_event(uint8_t player_no);
};
