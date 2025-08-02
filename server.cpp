#include <iostream>
#include "server.h"

Server::Server(const std::string& startPath) : startPath(startPath) {
    if (startPath.empty()) {
        this->startPath = "./";
    }
    std::cout << "Server initialized with start path: " << this->startPath << std::endl;
}