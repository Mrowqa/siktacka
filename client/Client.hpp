#pragma once

#include <common/network/HostAddress.hpp>
#include <common/network/TcpSocket.hpp>
#include <common/network/UdpSocket.hpp>
#include <common/protocol/MultipleGameEvent.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <unordered_set>


// Main class for siktacka-client
class Client final {
private:
    // command line arguments
    HostAddress gs_address;
    HostAddress gui_address;
    std::string player_name;

    // sockets
    UdpSocket gs_socket;
    TcpSocket gui_socket;

    // game state
    struct {
        uint32_t maxx = 0, maxy = 0;
        std::vector<std::string> players_names;
        MultipleGameEvent::Container events;
        uint32_t next_event_no = 0;
        bool game_over = false;
    } game_state;

    // client state
    struct {
        using system_clock = std::chrono::system_clock;

        uint64_t session_id = 0;
        uint32_t game_id = 0;
        std::unordered_set<uint32_t> prev_game_ids;
        system_clock::time_point next_hb_time = system_clock::now();
        system_clock::time_point last_server_response;
    } client_state;

    // GUI state
    struct {
        bool left_key_down = false;
        bool right_key_down = false;
        uint32_t next_event_no = 0;
    } gui_state;

public:
    Client(int argc, char *argv[]) noexcept;
    void run();

private:
    void parse_arguments(int argc, char *argv[]) noexcept;
    void init_client();
    void handle_gui_input();
    bool is_heartbeat_pending() const;
    void send_heartbeat();
    void send_updates_to_gui();
    bool pending_work() const;
    void receive_events_from_server();
    void handle_newly_received_events(MultipleGameEvent &new_events);
    void init_new_game(uint32_t new_game_id);
    void enqueue_events(MultipleGameEvent &events);
    void process_events();
    // Returns empty string on success, otherwise error message.
    std::string validate_game_event(const GameEvent &event);
};
