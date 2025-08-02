#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include "server.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	std::string pathArg = argv[1];

	if (argc > 1) {
		if (fs::exists(pathArg) && fs::is_directory(pathArg)) {
			std::cout << "Using provided path: " << pathArg << std::endl;

			Server server(pathArg);
		} else {
			std::cerr << "Provided path does not exist or is not a directory." << std::endl;
			return 1;
		}
	} else {
		std::cout << "No path provided, using default." << std::endl;
	}

	
	return 0;
}
