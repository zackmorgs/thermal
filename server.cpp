#include <iostream>
#include <string>

class Server {
public:
    Server(const std::string& startPath) {
        if (startPath.empty()) {
            this->startPath = "./";
        }
        std::cout << "Server initialized with start path: " << this->startPath << std::endl;
    }
private:
    std::string startPath;
};