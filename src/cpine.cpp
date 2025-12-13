#include <iostream>
#include <string>
#include <cstring>

#include "src/server.hpp"

#define PROJECT_NAME "cpine"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << argv[0] << " takes two arguments: <ip addr> <port> (-v)\n";
        return 1;
    }
    std::cout << "This is project " << PROJECT_NAME << ".\n";

    Server server(argv[1], argv[2], (argc >= 4 && (std::string)argv[3] == "-v"));

    if (int e = server.init(); e != 0) return e;

    return server.run();
}
