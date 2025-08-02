#pragma once
#include <string>

class Server {
public:
    Server(const std::string& startPath = "./");                   // Constructor

private:
    std::string startPath;           // Member variable
};