#include <client/Client.hpp>
#include <common/protocol/utils.hpp>
#include <common/protocol/HeartBeat.hpp>
#include <common/utils.hpp>

#include <iostream>
#include <cassert>
#include <thread>

using namespace std::chrono_literals;


static constexpr auto server_default_port = 12345;
static constexpr auto gui_default_hostname = "localhost";
static constexpr auto gui_default_port = 12346;

static constexpr long long min_port = 0;
static constexpr long long max_port = 65535;

static constexpr auto heartbeat_interval = 20ms;
static constexpr auto game_server_timeout = 1min;

static constexpr auto events_ahead_treshold = 100;
// TODO timeout for "previous events" (received event_no=X, but not X-1)

static constexpr auto socket_send_io_max_tries = 3;


static std::pair<std::string, unsigned short> with_default_port(std::string address, unsigned short port);
template<typename T, typename U, typename V>
static bool handle_socket_io(T op, U &&sock_name, V &&action, int tries_cnt = 1);


Client::Client(int argc, char *argv[]) noexcept {
    parse_arguments(argc, argv);
}


void Client::parse_arguments(int argc, char **argv) noexcept {
    if (argc < 3 || 4 < argc) {
        exit_with_error("Usage: ./siktacka-client player_name game_server_host[:port] [ui_server_host[:port]]");
    }

    player_name = argv[1];
    if (!validate_player_name(player_name)) {
        exit_with_error("Invalid player name");
    }

    auto address = with_default_port(argv[2], server_default_port);
    if (!gs_address.resolve(address.first, address.second)) {
        exit_with_error("Failed to resolve server address.");
    }
    std::cout << "Resolved server address as " << gs_address.to_string() << std::endl;

    address = with_default_port(argc == 4 ? argv[3] : gui_default_hostname, gui_default_port);
    if (!gui_address.resolve(address.first, address.second)) {
        exit_with_error("Failed to resolve GUI address.");
    }
    std::cout << "Resolved GUI address as " << gui_address.to_string() << std::endl;
}


void Client::run() {
    init_client();

    // Note: all functions does their work in loop until they finish or heartbeat is pending
    while (true) {
        if (is_heartbeat_pending()) {
            send_heartbeat();  // sending heartbeats to server is crucial
        }
        handle_gui_input();    // we want to be responsive to input

        // second part -> in case when GUI is malicious and sends as too many data...
        if (is_heartbeat_pending()) {
            send_heartbeat();
        } // and now following functions will have some time

        // since we want to prevent starvation at single step of events processing:
        send_updates_to_gui();  // first, we want to inform GUI about all processed events
        process_events();       // second, we want to process all received events
        receive_events_from_server();  // at last, we want to receive new events

        if (!pending_work()) {
            std::this_thread::sleep_for(0s);  // quit current time quantum if no more work
        }
    }
}


void Client::init_client() {
    if (gui_socket.connect(gui_address) != Socket::Status::Done) {
        exit_with_error("Failed to connect with GUI.");
    }
    std::cout << "Connected to GUI." << std::endl;

    if (gs_socket.init(gs_address.get()->ip_version) != Socket::Status::Done) {
        exit_with_error("Failed to create socket for game server communication.");
    }
    std::cout << "Created socket for game server communication." << std::endl;

    if (gui_socket.set_blocking(false) != Socket::Status::Done ||
            gs_socket.set_blocking(false) != Socket::Status::Done) {
        exit_with_error("Failed to make sockets non blocking.");
    }

    using namespace std::chrono;
    client_state.last_server_response = system_clock::now();
    client_state.session_id = duration_cast<microseconds>(
            high_resolution_clock::now().time_since_epoch()).count();
}


void Client::handle_gui_input() {
    std::string buffer;

    while (!is_heartbeat_pending()) {
        auto data_received = handle_socket_io([&buffer, this]() {
            return gui_socket.receive_line(buffer);
        }, "GUI", "receiving");

        if (!data_received) {
            return;
        }

        if (buffer == "LEFT_KEY_DOWN") {
            gui_state.left_key_down = true;
        }
        else if (buffer == "LEFT_KEY_UP") {
            gui_state.left_key_down = false;
        }
        else if (buffer == "RIGHT_KEY_DOWN") {
            gui_state.right_key_down = true;
        }
        else if (buffer == "RIGHT_KEY_UP") {
            gui_state.right_key_down = false;
        }
        else {
            std::cout << "Warning: received invalid message from GUI." << std::endl;
        }
    }
}


bool Client::is_heartbeat_pending() const {
    return client_state.next_hb_time <= std::chrono::system_clock::now();
}


void Client::send_heartbeat() {
    auto turn_direction = static_cast<int8_t>(
            (gui_state.left_key_down ? -1 : 0) + (gui_state.right_key_down ? 1 : 0));

    HeartBeat hb = {
            client_state.session_id,
            turn_direction,
            game_state.next_event_no,
            player_name,
    };

    if (!hb.validate()) {
        exit_with_error("Constructed invalid HeartBeat packet.");
    }

    client_state.next_hb_time = std::chrono::system_clock::now() + heartbeat_interval;
    auto data_sent = handle_socket_io([&hb, this]() {
        return this->gs_socket.send(hb.serialize(), this->gs_address);
    }, "Game server", "sending", socket_send_io_max_tries);

    if (!data_sent) {
        exit_with_error("Failed to sent data to game server (tried some times).");
    }
}


void Client::send_updates_to_gui() {
    while (!is_heartbeat_pending()) {
        if (gui_state.next_event_no == game_state.next_event_no) {
            return;
        }

        // move event out of collection; after sending it, we don't need it anymore
        auto event_ptr = std::move(game_state.events[gui_state.next_event_no]);
        gui_state.next_event_no++;
        assert(event_ptr != nullptr);
        assert(event_ptr->validate(GameEvent::Format::Text));

        auto data = event_ptr->serialize(GameEvent::Format::Text);

        auto data_sent = handle_socket_io([&data, this]() {
            return this->gui_socket.send_line(data);
        }, "GUI", "sending", socket_send_io_max_tries);

        if (!data_sent) {
            exit_with_error("Failed to sent data to GUI (tried some times).");
        }
    }
}


bool Client::pending_work() const {
    if (is_heartbeat_pending()) {
        return true;
    }

    // pending updates to be sent to GUI
    if (gui_state.next_event_no < game_state.next_event_no) {
        return true;
    }

    // pending events to be processed
    if (game_state.next_event_no < game_state.events.size()
            && game_state.events[game_state.next_event_no] != nullptr) {
        return true;
    }

    // TODO check if data awaiting on sockets (should not make real difference)

    return false;
}


void Client::receive_events_from_server() {
    std::string buffer;
    HostAddress src_addr;

    while (!is_heartbeat_pending()) {
        auto now = std::chrono::system_clock::now();

        auto data_received = handle_socket_io([&buffer, &src_addr, this]() {
            return this->gs_socket.receive(buffer, src_addr);
        }, "Game server", "receiving");

        if (!data_received) {
            if (client_state.last_server_response + game_server_timeout < now) {
                exit_with_error("Game server time out.");
            }
            continue;
        }

        if (src_addr != gs_address) {
            std::cout << "Info: received data from not-server." << std::endl;
            continue;
        }

        MultipleGameEvent new_events;
        if (!new_events.deserialize(buffer)) {
            std::cout << "Info: Received malformed data from server." << std::endl;
            continue;
        }

        client_state.last_server_response = now;
        handle_newly_received_events(new_events);
    }
}


void Client::handle_newly_received_events(MultipleGameEvent &new_events) {
    if (client_state.prev_game_ids.count(new_events.game_id) > 0) {
        return;
    }

    // Check events number
    auto first_no = new_events.events[0]->event_no;
    for (std::size_t i = 1; i < new_events.events.size(); i++) {
        if (new_events.events[i]->event_no != first_no + i) {
            exit_with_error("Error: received logically invalid data from server.");
        }
    }

    if (new_events.game_id != client_state.game_id) {
        init_new_game(new_events.game_id);
    }

    enqueue_events(new_events);
}


void Client::init_new_game(uint32_t new_game_id) {
    client_state.prev_game_ids.insert(client_state.game_id);
    client_state.game_id = new_game_id;

    game_state.maxx = game_state.maxy = 0;
    game_state.players_names.clear();
    game_state.events.clear();
    game_state.game_over = false;
    game_state.next_event_no = gui_state.next_event_no = 0;
}


void Client::enqueue_events(MultipleGameEvent &events) {
    auto &first_ev_ptr = events.events.front();
    auto &last_ev_ptr = events.events.back();

    if (last_ev_ptr->event_no < game_state.next_event_no
        || game_state.next_event_no + events_ahead_treshold < first_ev_ptr->event_no) {
        return;
    }

    if (game_state.events.size() <= last_ev_ptr->event_no) {
        game_state.events.resize(last_ev_ptr->event_no + 1);
    }

    for (auto &ev_ptr : events.events) {
        if (game_state.events[ev_ptr->event_no] != nullptr) {
            continue;
        }

        game_state.events[ev_ptr->event_no] = std::move(ev_ptr);
    }
}


void Client::process_events() {
    while (!is_heartbeat_pending()
           && game_state.next_event_no < game_state.events.size()
           && game_state.events[game_state.next_event_no] != nullptr) {
        auto &event = game_state.events[game_state.next_event_no];
        game_state.next_event_no++;

        auto error_msg = validate_game_event(*event);
        if (!error_msg.empty()) {
            exit_with_error(std::move(error_msg));
        }

        // process new game event
        if (event->type == GameEvent::Type::NewGame) {
            auto &data = event->new_game_data;
            game_state.maxx = data.maxx;
            game_state.maxy = data.maxy;
            game_state.players_names = data.players_names;
            game_state.game_over = false;
        }
    }
}


std::string Client::validate_game_event(const GameEvent &event) {
    if (event.event_no == 0 && event.type != GameEvent::Type::NewGame) {
        return "Server error: first game event is not NewGame.";
    }

    if (event.event_no > 0 && event.type == GameEvent::Type::NewGame) {
        return "Server error: got NewGame event with id != 0.";
    }

    if (game_state.game_over) {
        return "Server error: got event after game over.";
    }

    switch (event.type) {
        case GameEvent::Type::NewGame:
            break;

        case GameEvent::Type::Pixel:
            if (event.pixel_data.player_no >= game_state.players_names.size()) {
                return "Server error: got player_no higher than number of players.";
            }
            if (event.pixel_data.x >= game_state.maxx || event.pixel_data.y >= game_state.maxy) {
                return "Server error: got pixel outside the map.";
            }
            break;

        case GameEvent::Type::PlayerEliminated:
            if (event.player_eliminated_data.player_no >= game_state.players_names.size()) {
                return "Server error: got player_no higher than number of players.";
            }
            break;

        case GameEvent::Type::GameOver:
            break;
    }

    return "";
}


// --------------------------- helper methods
std::pair<std::string, unsigned short> with_default_port(std::string address, unsigned short port) {
    auto divider_pos = address.rfind(':');
    if (divider_pos == std::string::npos) {
        return std::make_pair(address, port);
    }

    if (address.find(':') < divider_pos) {
        // Assuming IPv6
        return std::make_pair(address, port);
    }

    auto host = address.substr(0, divider_pos);

    // If custom port:
    if (divider_pos < address.length() - 1) {
        try {
            auto port_num = from_string<long long>(address.substr(divider_pos + 1));
            if (port_num < min_port || max_port < port_num) {
                exit_with_error("Invalid port number.");
            }

            port = static_cast<unsigned short>(port_num);
        }
        catch (std::exception &exc) {
            exit_with_error("Invalid port number.");
        }
    }

    return std::make_pair(host, port);
}


template<typename T, typename U, typename V>
static bool handle_socket_io(T op, U &&sock_name, V &&action, int tries_cnt) {
    int attempts_done = 0;
    while (attempts_done++ < tries_cnt) {
        auto status = op();
        switch (status) {
            case Socket::Status::Done:
                return true;
                break;

            case Socket::Status::NotReady:
                return false;
                break;

            case Socket::Status::Error: {
                std::ostringstream str;
                str << std::forward<U&&>(sock_name) << " socket error occurred while "
                    << std::forward<V&&>(action) << " data.";
                exit_with_error(str.str());
                break;
            }

            case Socket::Status::Disconnected: {
                std::ostringstream str;
                str << std::forward<U&&>(sock_name) << " socket disconnected.";
                exit_with_error(str.str());
                break;
            }

            case Socket::Status::Partial:
            default:
                exit_with_error("Socket I/O operation returned invalid status.");
                break;
        }
    }

    return false;
}