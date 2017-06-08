#include <server/Server.hpp>
#include <common/utils.hpp>
#include <common/protocol/MultipleGameEvent.hpp>

#include <algorithm>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;


// TODO adjust two constants below
static constexpr auto max_rounds_per_second = 1'000;
static constexpr auto max_map_dimension = 10'000;

static constexpr auto max_connected_clients = 42;
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
                            "-t", argv[i + 1], 1);
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
    init_server();

    while (true) {
        check_clients_connections(); // delete TODO
        do { // todo napisać uzasadnienie tego designu
            handle_clients_input();
            send_events_to_clients();

            if (!pending_work()) {
                std::this_thread::sleep_for(0s);  // quit current time quantum if no more work
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
              << " ---->  Server port: " << config.port_number << "  <----" << std::endl
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
        if (it->second.last_heartbeat_time + client_timeout < now) {
            it = disconnect_client(it);
        }
        else {
            it++;
        }
    }
}


Server::ClientContainer::iterator Server::disconnect_client(
        Server::ClientContainer::iterator client) {
    std::cout << "Player disconnected: " << client->second.name << "." << std::endl;

    bool replace_next = server_state.next_client == client;
    client = server_state.clients.erase(client);
    if (replace_next) {
        server_state.next_client = client;
    }

    return client;
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
        auto &client = server_state.next_client->second;
        if (client.last_heartbeat_time + client_timeout < now) {
            disconnect_client(server_state.next_client);
            continue;
        }

        if (client.next_event_no == game_state.serialized_events.size()) {
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


// --------------------------------------- helpers
template<typename T, typename It>
static It advance_iterator_circularly(T &container, It it) {
    ++it;
    if (it == container.end()) {
        it = container.begin();
    }

    return it;
}
