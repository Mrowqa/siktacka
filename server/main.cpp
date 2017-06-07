#include <server/Server.hpp>

#include <iostream>


int main(int argc, char *argv[]) {
    try {
        Server server(argc, argv);
        server.run();
    }
    catch (std::exception &exc) {
        std::cerr << "Error occured: " << exc.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unkown error occured." << std::endl;
    }

    return 0;
}
