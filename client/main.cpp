#include <client/Client.hpp>

#include <iostream>


int main(int argc, char *argv[]) {
    try {
        Client client(argc, argv);
        client.run();
    }
    catch (std::exception &exc) {
        std::cerr << "Error occured: " << exc.what() << std::endl;
    }
    return 0;
}
