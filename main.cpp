#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>

#include "server.h"

namespace fs = std::filesystem;

int main()
{
	Server server = new Server("./");
	return 0;
}
