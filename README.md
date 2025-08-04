
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
    - `cd ./build; cmake --build .`
- Run the executable
    - `./Debug/thermal.exe "C:\path\to\your\server\root" -w`

Also available to build in Visual Studio