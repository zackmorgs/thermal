#include <iostream>
#include "server.h"

Server::Server() {
    startPath = "./";
    std::cout << "Server initialized with start path: " << startPath << std::endl;
}