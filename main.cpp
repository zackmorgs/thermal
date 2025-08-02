#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include "server.cpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Error: No path provided. Please specify a directory path." << std::endl;
		std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
		return 1;
	}

	std::string pathArg = argv[1];

	if (fs::exists(pathArg) && fs::is_directory(pathArg)) {
		std::cout << "Using provided path: " << pathArg << std::endl;
		Server server(pathArg);
	} else {
		std::cerr << "Provided path does not exist or is not a directory." << std::endl;
		return 1;
	}

	return 0;
}
