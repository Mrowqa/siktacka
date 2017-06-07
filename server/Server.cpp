#include <server/Server.hpp>
#include <common/utils.hpp>

#include <cstring>
#include <thread>

using namespace std::chrono_literals;


static constexpr auto max_rounds_per_second = 1'000;
static constexpr auto max_map_dimension = 10'000;


static constexpr auto client_timeout = 2s;


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
        handle_clients_input();
        send_events_to_clients(); // todo note: send packet after packet to all, not to starve anyone
        if (game_update_pending()) {
            update_game_state();
        }

        if (!pending_work()) {
            std::this_thread::sleep_for(0s);  // quit current time quantum if no more work
        }
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
}


void Server::handle_clients_input() {
    // todo
}


void Server::send_events_to_clients() {
    // todo
}


void Server::update_game_state() {
    // todo
}


bool Server::game_update_pending() const {
    // todo
    return false;
}


bool Server::pending_work() const {
    // todo
    return false;
}
