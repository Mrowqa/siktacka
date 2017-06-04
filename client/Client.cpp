#include "Client.hpp"
#include <common/protocol/utils.hpp>
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
static constexpr auto server_timeout = 1min;


static std::pair<std::string, unsigned short> with_default_port(std::string address, unsigned short port);


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

    while (true) {
        handle_gui_input();
        if (is_heartbeat_pending()) {
//            send_heartbeat();
        }
//        send_updates_to_gui();
//        receive_server_updates();
//
//        if (pending_work()) {
            std::this_thread::sleep_for(0s);
//        }
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

    client_state.last_server_response = std::chrono::system_clock::now();
}


void Client::handle_gui_input() {
    std::string buffer;

    while (!is_heartbeat_pending()) {
        auto status = gui_socket.receive_line(buffer);
        switch (status) {
            case Socket::Status::Done:
                break;

            case Socket::Status::NotReady:
                return;
                break;

            case Socket::Status::Error:
                exit_with_error("GUI socket error occurred while receiving data.");
                break;

            case Socket::Status::Disconnected:
                exit_with_error("GUI disconnected.");
                break;

            case Socket::Status::Partial:
            default:
                assert(false);
                break;
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


// --------------------------- TODO delete it! -- trash code --------------
//#include <cerrno>
//#include <cstring>
//void Client::run() noexcept {
//    UdpSocket s;
//    if (s.init(gs_address.get()->ip_version) != Socket::Status::Done ||
//        s.set_blocking(true) != Socket::Status::Done) {
//        exit_with_error("Failed to initialize socket.");
//    }
//    /*if (s.init(gs_address.get()->ip_version) != Socket::Status::Done ||
//            s.bind(gs_address) != Socket::Status::Done ||
//            s.set_blocking(true) != Socket::Status::Done) {
//        exit_with_error("Failed to initialize socket.");
//    }*/
//
//
//    auto status_to_string = [](Socket::Status status) {
//        switch (status) {
//            case Socket::Status::Done: return "Done";
//            case Socket::Status::Error: return "Error";
//            case Socket::Status::Partial: return "Partial";
//            case Socket::Status::Disconnected: return "Disconnected";
//            case Socket::Status::NotReady: return "NotReady";
//            default: return "Unknown";
//        }
//    };
//
//
//    std::string buf;
//    HostAddress src_addr;
//    Socket::Status status;
//    while (true) {
//        std::cout << "Sending: Hello\n";
//        status = s.send("Hello\n", gs_address);
//        if (status != Socket::Status::Done) {
//            std::cout << "status: " << status_to_string(status) << "\n";
//            std::cout << "errno: " << strerror(errno) << "\n";
//            exit(1);
//        }
//        std::cout << "Receiving: ";
//        status = s.receive(buf, src_addr);
//        if (status != Socket::Status::Done) {
//            std::cout << "status: " << status_to_string(status) << "\n";
//            std::cout << "errno: " << strerror(errno) << "\n";
//            exit(1);
//        }
//        std::cout << buf;
//        break;
//    }
//}

//#include <cerrno>
//#include <cstring>
//void Client::run() noexcept {
//    TcpSocket s;
//    if (s.connect(gs_address) != Socket::Status::Done ||
//        s.set_blocking(true) != Socket::Status::Done) {
//        exit_with_error("Failed to initialize socket.");
//    }
//    /*if (s.init(gs_address.get()->ip_version) != Socket::Status::Done ||
//            s.bind(gs_address) != Socket::Status::Done ||
//            s.set_blocking(true) != Socket::Status::Done) {
//        exit_with_error("Failed to initialize socket.");
//    }*/
//
//
//    auto status_to_string = [](Socket::Status status) {
//        switch (status) {
//            case Socket::Status::Done: return "Done";
//            case Socket::Status::Error: return "Error";
//            case Socket::Status::Partial: return "Partial";
//            case Socket::Status::Disconnected: return "Disconnected";
//            case Socket::Status::NotReady: return "NotReady";
//            default: return "Unknown";
//        }
//    };
//
//
//    std::string buf;
//    std::size_t sent = 0;
//    Socket::Status status;
//    while (true) {
//        std::cout << "Sending: Hello\n";
//        //buf = "Hello!\n";
//        //status = s.send(buf, sent);
//        status = s.send_line("line!");
//        if (status != Socket::Status::Done) {
//            std::cout << "status: " << status_to_string(status) << "\n";
//            std::cout << "errno: " << strerror(errno) << "\n";
//            exit(1);
//        }
//        std::cout << "Receiving: ";
//        //status = s.receive(buf, 10);
//        status = s.receive_line(buf);
//        if (status != Socket::Status::Done) {
//            std::cout << "status: " << status_to_string(status) << "\n";
//            std::cout << "errno: " << strerror(errno) << "\n";
//            exit(1);
//        }
//        std::cout << buf;
//        break;
//    }
//}