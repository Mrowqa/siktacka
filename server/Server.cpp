#include <server/Server.hpp>
#include <common/utils.hpp>
#include <common/protocol/MultipleGameEvent.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;


// TODO adjust first two constants below
static constexpr auto max_rounds_per_second = 1'000;
static constexpr auto max_map_dimension = 10'000;
static constexpr auto max_turning_speed = 359;

static constexpr auto deg_to_rad = M_PI / 180.;
static constexpr auto max_connected_clients = 42;
static constexpr auto min_players_number = 2;
static constexpr auto client_timeout = 2s;

template<typename T, typename It>
static It advance_iterator_circularly(T &container, It it) ;


Server::Server(int argc, char *argv[]) {
    parse_arguments(argc, argv);
}


void Server::parse_arguments(int argc, char *argv[]) {
    for (auto i = 1; i < argc; i += 2) {
        if (strlen(argv[i]) != 2 || argv[i][0] != '-') {
            print_usage(argv[0]);
            exit_with_error("Invalid option: " + std::string(argv[i]));
        }

        auto opt = argv[i][1];
        if (opt != 'W' && opt != 'H' && opt != 'p' && opt != 's' && opt != 't' && opt != 'r') {
            print_usage(argv[0]);
            exit_with_error("Unknown option: " + std::string(argv[i]));
        }

        if (i + 1 >= argc) {
            print_usage(argv[0]);
            exit_with_error("Missing argument for option: " + std::string(argv[i]));
        }

        try {
            switch (opt) {
                case 'W':
                    config.map_width = to_number<decltype(config.map_width)>(
                            "-W", argv[i + 1], 1, max_map_dimension);
                    break;

                case 'H':
                    config.map_height = to_number<decltype(config.map_height)>(
                            "-H", argv[i + 1], 1, max_map_dimension);
                    break;

                case 'p':
                    config.port_number = to_number<decltype(config.port_number)>(
                            "-p", argv[i + 1], HostAddress::min_port, HostAddress::max_port);
                    break;

                case 's':
                    config.rounds_per_second = to_number<decltype(config.rounds_per_second)>(
                            "-s", argv[i + 1], 1, max_rounds_per_second);
                    break;

                case 't':
                    config.turning_speed = to_number<decltype(config.turning_speed)>(
                            "-t", argv[i + 1], 1, max_turning_speed);
                    break;

                case 'r':
                    auto seed = to_number<uint64_t>("-r", argv[i + 1]);
                    server_state.rand_gen.set_seed(seed);
                    break;
            }
        }
        catch (std::exception &exc) {
            print_usage(argv[0]);
            exit_with_error(exc.what());
        }
    }
}


void Server::print_usage(const char *name) const noexcept {
    std::cerr << "Usage: " << name << " [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]" << std::endl;
}


void Server::run() {
    // Loop design:
    //
    //    * handle_clients_input() and send_events_to_clients() does at most one network
    //      I/O, so work is balanced between receiving input (+ client session management)
    //      and sending events to clients.
    //
    //    * send_events_to_clients() sends only one datagram to client and iterates
    //      to a next one. This way, in case of long game and a new client joining
    //      the server, this client does not have higher priority. In fact, all clients
    //      have the same priority.
    //
    //    * if there is no work to do (i.a. datagrams to be send and pending game update),
    //      the server sleeps for a while to avoid burning CPU cycles uselessly.
    //
    // Note: in case for UPDATES_PER_SECOND = 1 and tests for exactly 2s timeout, clients
    //       timeouts are being checked in check_clients_connections() and before sending
    //       datagram in send_events_to_clients(). In fact, one of these methods would be
    //       enough, preferably the lazy one (in send_events_to_clients()).

    init_server();

    while (true) {
        check_clients_connections();
        do {
            handle_clients_input();
            send_events_to_clients();

            if (!pending_work()) {
                std::this_thread::sleep_for(1ms);  // sleep a little bit if no more work
            }
        } while (!game_update_pending());

        update_game_state();
    }
}


void Server::init_server() {
    std::cout << "------------- Server configuration -------------" << std::endl
              << "         Dimensions: " << config.map_width << " x " << config.map_height << std::endl
              << "  Rounds per second: " << config.rounds_per_second << std::endl
              << "      Turning speed: " << config.turning_speed << std::endl
              << "        Server port: " << config.port_number << std::endl
              << "        Random seed: " << server_state.rand_gen.peek() << std::endl
              << "------------------------------------------------" << std::endl
              << std::endl;

    HostAddress address;
    if(!address.resolve("::", config.port_number) ||
            address.get()->ip_version != HostAddress::IpVersion::IPv6 ||
            socket.init(address.get()->ip_version) != Socket::Status::Done ||
            socket.bind(address) != Socket::Status::Done ||
            socket.set_blocking(false) != Socket::Status::Done) {
        exit_with_error("Failed to initialize server socket.");
    }

    game_state.map.resize(config.map_height * config.map_width);
    server_state.next_client = server_state.clients.begin();
}


void Server::check_clients_connections() {
    auto it = server_state.clients.begin();
    auto now = std::chrono::system_clock::now();

    while (it != server_state.clients.end()) {
        auto next = it;
        next++;
        if (it->second.last_heartbeat_time + client_timeout < now) {
            disconnect_client(it);
        }

        it = next;
    }
}


void Server::disconnect_client(Server::ClientContainer::iterator client) {
    std::cout << "Player disconnected: " << client->second.name << "." << std::endl;

    bool replace_next = server_state.next_client == client;
    client = server_state.clients.erase(client);
    if (replace_next) {
        server_state.next_client = client == server_state.clients.end()
                                   ? client = server_state.clients.begin() : client;
    }
}


void Server::handle_clients_input() {
    if (game_update_pending()) {
        return;
    }

    HostAddress client_addr;
    std::string buffer;

    auto status = socket.receive(buffer, client_addr);
    if (status != Socket::Status::Done) {
        return;  // no data or socket error
    }

    HeartBeat hb;
    if (!hb.deserialize(buffer)) {
        return;
    }

    auto client_it = handle_client_session(client_addr, hb);
    if (client_it == server_state.clients.end()) {
        return;
    }

    if (server_state.clients.size() == 1) {
        server_state.next_client = server_state.clients.begin();
    }

    auto &client = client_it->second;

    if (!client.ready_to_play && !game_state.game_in_progress && hb.turn_direction != 0) {
        std::cout << "Client " << client.name << " is ready." << std::endl;
        client.ready_to_play = true;
    }

    if (client.player_no != -1) {
        game_state.players[client.player_no].turn_direction = hb.turn_direction;
    }
}


Server::ClientContainer::iterator Server::handle_client_session(
        const HostAddress &client_addr, const HeartBeat &hb) {
    bool new_session = false;
    auto client_it = server_state.clients.find(client_addr);

    if (client_it == server_state.clients.end()) {
        // new client
        if (server_state.clients.size() >= max_connected_clients) {
            // TODO log not too often
            std::cout << "Rejecting player " << hb.player_name
                      << ": maximum number of clients reached." << std::endl;
            return server_state.clients.end();
        }

        if (!check_name_availability(hb.player_name)) {
            // TODO log not too often
            std::cout << "Rejecting player " << hb.player_name
                      << ": name already in use." << std::endl;
            return server_state.clients.end();
        }

        std::cout << "New player connected: " << hb.player_name << "." << std::endl;
        new_session = true;

        auto &client = server_state.clients[client_addr];
        client.address.set(*client_addr.get());
        client_it = server_state.clients.find(client_addr);
    }
    else {
        // known client
        auto &client = client_it->second;
        if (client.session_id != hb.session_id) {
            if (client.name != hb.player_name && !check_name_availability(hb.player_name)) {
                disconnect_client(client_it);
                return server_state.clients.end();
            }

            std::cout << "Player " << client.name << " initialized new session as "
                      << hb.player_name << "." << std::endl;
            new_session = true;
        }
        else if (client.name != hb.player_name) {
            return server_state.clients.end();  // drop invalid packet
        }
    }

    auto &client = client_it->second;
    if (new_session) {
        client.session_id = hb.session_id;
        client.name = hb.player_name;
        client.player_no = -1;
        client.watching_game = game_state.game_in_progress;
        client.ready_to_play = false;
        // New clients should ask for event no 0.
        client.got_new_game_event = true;
    }

    client.last_heartbeat_time = std::chrono::system_clock::now();
    client.next_event_no = hb.next_expected_event_no;

    return client_it;
}


bool Server::check_name_availability(const std::string &name) const noexcept {
    return std::all_of(server_state.clients.begin(), server_state.clients.end(),
                       [&name](const auto &client_session) {
        return name != client_session.second.name;
    });
}


void Server::send_events_to_clients() {
    if (game_update_pending()) {
        return;
    }

    auto clients_num = server_state.clients.size();
    auto now = std::chrono::system_clock::now();

    for (std::size_t i = 0; i < clients_num; i++) {
        assert(server_state.next_client != server_state.clients.end());
        auto &client = server_state.next_client->second;
        if (client.last_heartbeat_time + client_timeout < now) {
            disconnect_client(server_state.next_client); // advances circularly next_client
            continue;
        }

        if (client.watching_game && !client.got_new_game_event) {
            client.next_event_no = 0;
        }

        if (!client.watching_game
                || client.next_event_no >= game_state.serialized_events.size()) {
            server_state.next_client = advance_iterator_circularly(
                    server_state.clients, server_state.next_client);
            continue;
        }

        MultipleGameEvent mge;
        mge.game_id = game_state.game_id;
        auto data_and_offset = mge.prepare_packet_from_cache(
                game_state.serialized_events, client.next_event_no);

        if (socket.send(data_and_offset.first,
                        server_state.next_client->first) == Socket::Status::Done) {
            if (client.next_event_no == 0) {
                client.got_new_game_event = true;
            }
            client.next_event_no = data_and_offset.second;
        }
        // we are intentionally ignoring errors here

        server_state.next_client = advance_iterator_circularly(
                server_state.clients, server_state.next_client);
        return;
    }
}


void Server::update_game_state() {
    game_state.next_update_time += 1'000'000us / config.rounds_per_second;

    if (game_state.game_in_progress) {
        update_lasting_game_state();
    }
    else {
        start_new_game_if_possible();
    }
}


void Server::update_lasting_game_state() {
    for (std::size_t ind = 0; ind < game_state.players.size(); ind++) {
        auto &player = game_state.players[ind];
        if (!player.alive) {
            continue;
        }

        uint32_t last_x = player.pos_x;
        uint32_t last_y = player.pos_y;

        if (player.turn_direction == -1) {
            player.angle -= config.turning_speed;
            if (player.angle < 0) {
                player.angle += 360;
            }
        }
        else if (player.turn_direction == 1) {
            player.angle += config.turning_speed;
            if (player.angle >= 360) {
                player.angle -= 360;
            }
        }

        player.pos_x += cos(player.angle * deg_to_rad);
        player.pos_y += sin(player.angle * deg_to_rad);

        uint32_t new_x = player.pos_x;
        uint32_t new_y = player.pos_y;

        if (last_x == new_x && last_y == new_y) {
            continue;
        }

        if (!is_on_map(player.pos_x, player.pos_y) || map_get(new_x, new_y)) {
            if (handle_player_eliminated_event(ind)) {
                return;  // game over
            }
        }
        else {
            handle_pixel_event(ind, new_x, new_y);
        }
    }
}


void Server::start_new_game_if_possible() {
    // TODO split into smaller functions
    uint8_t counter = 0;
    std::vector<std::string> names;

    // Validate if all clients with non-empty login are ready
    for (auto &client : server_state.clients) {
        if (client.second.name.empty()) {
            continue;
        }

        if (!client.second.ready_to_play) {
            return;
        }

        counter++;
        names.push_back(client.second.name);
    }

    if (counter < min_players_number) {
        return;
    }

    std::cout << "Starting new game." << std::endl;

    std::sort(names.begin(), names.end());

    // Crop the player list if they do not fit into one UDP datagram
    GameEvent ev;
    ev.type = GameEvent::Type::NewGame;
    ev.new_game_data.maxx = config.map_width;
    ev.new_game_data.maxy = config.map_height;
    ev.new_game_data.players_names = std::move(names);

    auto &pl_names = ev.new_game_data.players_names;
    auto names_size = ev.new_game_data.calculate_used_names_capacity();
    while (ev.new_game_data.names_capacity < names_size) {
        names_size -= pl_names.size() + 1;
        pl_names.pop_back();
    }

    // Emit NewGame event and map clients to players
    game_state.game_id = server_state.rand_gen.next();
    game_state.serialized_events.clear();
    game_state.game_in_progress = true;
    emit_game_event(ev);
    game_state.players.resize(pl_names.size());
    game_state.alive_players_cnt = game_state.players.size();

    for (auto &client : server_state.clients) {
        client.second.watching_game = true;
        client.second.got_new_game_event = false;
        client.second.ready_to_play = false;
        if (client.second.name.empty()) {
            continue;
        }

        auto it = std::lower_bound(pl_names.begin(), pl_names.end(),
                                   client.second.name);
        if (*it != client.second.name) {
            continue;  // too many players, this one is not lucky
        }

        auto ind = it - pl_names.begin();
        client.second.player_no = ind;
        game_state.players[ind].name = client.second.name;
    }

    // Init map
    game_state.map.resize(0);
    game_state.map.resize(config.map_height * config.map_width, false);

    // Init players
    for (std::size_t ind = 0; ind < game_state.players.size(); ind++) {
        auto &player = game_state.players[ind];

        player.turn_direction = 0;
        player.alive = true;
        player.pos_x = (server_state.rand_gen.next() % config.map_width) + 0.5;
        player.pos_y = (server_state.rand_gen.next() % config.map_height) + 0.5;
        player.angle = server_state.rand_gen.next() % 360;

        uint32_t x = player.pos_x;
        uint32_t y = player.pos_y;

        if (map_get(x, y)) {
            if (handle_player_eliminated_event(ind)) {
                return; // game over
            }
        }
        else {
            handle_pixel_event(ind, x, y);
        }
    }
}


bool Server::is_on_map(double x, double y) const {
    return 0 <= x && x < config.map_width &&
           0 <= y && y < config.map_height;
}


void Server::map_set(uint32_t x, uint32_t y) {
    game_state.map[y * config.map_width + x] = true;
}


bool Server::map_get(uint32_t x, uint32_t y) const {
    return game_state.map[y * config.map_width + x];
}


bool Server::game_update_pending() const {
    return game_state.next_update_time <= std::chrono::system_clock::now();
}


bool Server::pending_work() const {
    if (game_update_pending()) {
        return true;
    }

    auto events_number = game_state.serialized_events.size();
    if (std::any_of(server_state.clients.begin(), server_state.clients.end(),
                    [events_number](const auto &client) {
        return client.second.watching_game &&
                client.second.next_event_no < events_number;
    })) {
        return true;
    }

    // TODO check if data awaiting on sockets (should not make real difference)

    return false;
}


void Server::handle_pixel_event(uint8_t player_no, uint32_t x, uint32_t y) {
    GameEvent ev;
    ev.type = GameEvent::Type::Pixel;
    ev.pixel_data.player_no = player_no;
    ev.pixel_data.x = x;
    ev.pixel_data.y = y;
    emit_game_event(ev);

    map_set(x, y);
}


bool Server::handle_player_eliminated_event(uint8_t player_no) {
    GameEvent ev;
    ev.type = GameEvent::Type::PlayerEliminated;
    ev.player_eliminated_data.player_no = player_no;
    emit_game_event(ev);
    std::cout << "Player eliminated: " << game_state.players[player_no].name
              << std::endl;

    game_state.players[player_no].alive = false;
    game_state.alive_players_cnt--;
    if (game_state.alive_players_cnt <= 1) {
        game_state.game_in_progress = false;
        ev.type = GameEvent::Type::GameOver;
        emit_game_event(ev);
        std::cout << "Game over." << std::endl;

        return true;
    }

    return false;
}


void Server::emit_game_event(GameEvent &event) {
    event.event_no = game_state.serialized_events.size();
    if (!event.validate(GameEvent::Format::Binary)) {
        std::cout << "Warning: Tried to emit invalid game event. "
                  << "Dropping it." << std::endl;
    }

    game_state.serialized_events.emplace_back(
            event.serialize(GameEvent::Format::Binary));
}


// --------------------------------------- helpers
template<typename T, typename It>
static It advance_iterator_circularly(T &container, It it) {
    ++it;
    if (it == container.end()) {
        it = container.begin();
    }

    return it;
}
