
<div align="left">
  <img src="./public/img/svg/thermal_logo.svg" alt="Thermal Logo" width="100" height="100">
</div>

# thermal


I haven't coded in C++ in a while, so this is me messing around with it for the first time in like 5 years. 

## Demonstration
- [YouTube Demonstration](https://youtu.be/0NOJlBduEfc)

## Features
- Fast static C++ server
- Hot-Reload capability
- [h5bp](https://github.com/h5bp/html5-boilerplate) boilerplate in `./public`

### Project Goals (To-Do)
- https server
- SPA mode
- hot-reload
- extendable REST api intergration

### Instructions
- Install cmake:
    - `choco install cmake`
- Clone the repo
    - `git clone https://github.com/zackmorgs/thermal.git ./my-project`
- Compile it:
    - `cd ./my-project/build; cmake --build .`
### Usage
- Run the server:
    - `./Debug/thermal.exe "C:\path\to\your\server\root"`
- Run with watch mode for hot-reload:
    - `./Debug/thermal.exe "C:\path\to\your\server\root" -w`
- Run on a custom port:
    - `./Debug/thermal.exe "C:\path\to\your\server\root" -p 3000`
- Combine watch mode and custom port:
    - `./Debug/thermal.exe "C:\path\to\your\server\root" -w -p 3000`

### Command Line Options
- `-w` : Enable watch mode for hot-reload (auto-refresh browser on file changes)
- `-p <port>` : Specify port number (default: 8080, range: 1-65535)

Also available to build in Visual Studio