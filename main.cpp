#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <vector>

#include "server.cpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Error: No path provided. Please specify a directory path." << std::endl;
		std::cerr << "Usage: " << argv[0] << " [-w] <directory_path>" << std::endl;
		std::cerr << "  -w: Enable watch mode" << std::endl;
		return 1;
	}

	bool watchMode = false;
	std::string pathArg;

	// Parse arguments
	std::vector<std::string> args(argv + 1, argv + argc);
	
	for (size_t i = 0; i < args.size(); ++i) {
		if (args[i] == "-w") {
			watchMode = true;
		} else if (pathArg.empty()) {
			pathArg = args[i];
		}
	}

	if (pathArg.empty()) {
		std::cerr << "Error: No directory path provided." << std::endl;
		std::cerr << "Usage: " << argv[0] << " [-w] <directory_path>" << std::endl;
		return 1;
	}

	if (fs::exists(pathArg) && fs::is_directory(pathArg)) {
		std::cout << "Using provided path: " << pathArg << std::endl;
		if (watchMode) {
			std::cout << "Watch mode enabled" << std::endl;
		}
		
		Server server(pathArg, watchMode);
		
		if (watchMode) {
			std::cout << "Starting watch mode... Press Ctrl+C to exit." << std::endl;
			server.startWatching();
		}
	} else {
		std::cerr << "Provided path does not exist or is not a directory." << std::endl;
		return 1;
	}

	return 0;
}
