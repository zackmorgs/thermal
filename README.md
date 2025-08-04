
# thermal
A Unix/MacOS HTTP server with hot-reload capability, written in C++.

## Demonstration
- [YouTube Demonstration](https://youtu.be/0NOJlBduEfc)

## Features
- 🚀 Fast static C++ server
- 🔥 Hot-reload capability with real-time file watching
- 🌐 Cross-platform support (Linux, Windows, macOS)
- 📦 [h5bp](https://github.com/h5bp/html5-boilerplate) boilerplate in `./public`
- ⚡ Multi-threaded request handling
- 🔧 Configurable port and directory serving

### Project Goals (To-Do)
- 🔒 HTTPS server support
- 📱 SPA (Single Page Application) mode
- 🛠️ Extendable REST API integration
- 🗂️ Enhanced file serving optimizations

## Quick Setup for Ubuntu

### Automatic Setup
Run the setup script to install dependencies and configure the environment:
```bash
./setup-ubuntu.sh
```

### Manual Setup
1. **Install dependencies:**
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake ninja-build gcc g++
   ```

2. **Clone and build:**
   ```bash
   git clone https://github.com/zackmorgs/thermal.git ./my-project-name
   cd ./my-project-name
   mkdir build && cd build
   cmake ..
   make
   ```

## Usage
(if current directory is ./build/bin)

### Basic Usage
```bash
# Serve files from a directory
./thermal /path/to/your/web/files

# Serve files from the public directory (example)
./thermal ../public
```

### With Hot-Reload
```bash
# Enable hot-reload for development (using provided public folder)
./thermal -w ./../../public
```

### Custom Port
```bash
# Run on a custom port
./thermal -p 3000 ./../../public

# Combine hot-reload and custom port
./thermal -w -p 3000 ./../../public
```

### Command Line Options
- `-w` : Enable watch mode for hot-reload (auto-refresh browser on file changes)
- `-p <port>` : Specify port number (default: 8080, range: 1-65535)
- `<directory>` : Path to the directory to serve (required)

## Platform Support

### Ubuntu/Linux
- ✅ Full native support
- ✅ POSIX sockets implementation
- ✅ GCC/Clang compilation
- ✅ Package manager integration

### macOS
- ✅ POSIX sockets (same as Linux)
- ✅ Clang compilation

### Windows
- Available via WSL
    - programmed in this environment

## Development

### Building with Different Configurations

#### Debug Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

#### Release Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

#### Using CMake Presets (Ubuntu)
```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

### Project Structure
```
thermal/
├── src/
│   ├── main.cpp              # Entry point
│   └── server/
│       ├── server.h          # Server interface
│       ├── server.cpp        # Core server implementation
│       ├── server_optimized.h # Optimized server interface
│       └── optimizations.cpp # Performance optimizations
├── public/                   # Example web files
├── CMakeLists.txt           # Build configuration
├── CMakePresets.json        # Build presets
└── setup-ubuntu.sh         # Ubuntu setup script
```

## Contributing

This project serves as a learning experience for modern C++ and cross-platform development. Contributions are welcome!