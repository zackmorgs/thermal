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
	// if there are less than 2 arguments, print an error message
	// this includes the program name and the path argument
	if (argc < 2) {
		std::cerr << "Error: No path provided. Please specify a directory path." << std::endl;
		std::cerr << "Usage: " << argv[0] << " [-w] <directory_path>" << std::endl;
		std::cerr << "  -w: Enable watch mode" << std::endl;
		return 1;
	}
	
	// initialize watch mode, root server path string
	bool watchMode = false;
	std::string pathArg;
	
	// Parse arguments into a vector of strings
	std::vector<std::string> args(argv + 1, argv + argc);
	
	// iterate through all arguments, looking for the watch flag and path
	for (size_t i = 0; i < args.size(); ++i) {
		if (args[i] == "-w") {
			watchMode = true;
		} else {
			// arg is something other than "-w", we hope its a path
			// Verify the path exists and is a directory
			if (fs::exists(args[i]) && fs::is_directory(args[i])) {
				std::cout << "Using provided path: " << args[i] << std::endl;
				pathArg = args[i];
			} else {
				// argument is not a path
				std::cerr << "Error: Invalid directory path provided." << std::endl;
				std::cerr << "Provide invalid input: " << args[i] << std::endl;
				return 1;
			}
		}
	}
	
	// Check if we found a path
	if (!pathArg.empty()) {
		// if path is found, create a server
		Server server(pathArg, watchMode);		
	} else {
		std::cerr << "Error: No directory path provided." << std::endl;
		std::cerr << "Usage: " << argv[0] << " [-w] <directory_path>" << std::endl;
		return 1;
	}
	

	
	return 0;
}
