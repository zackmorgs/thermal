# CMakeList.txt : CMake project for thermal server
cmake_minimum_required(VERSION 3.15)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("thermal" VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/server/server.cpp
)

# Headers
set(HEADERS
    src/server/server.h
)

# Create executable
add_executable(thermal ${SOURCES} ${HEADERS})

# Windows-specific libraries
if(WIN32)
    target_link_libraries(thermal ws2_32)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(thermal PRIVATE /W4)
else()
    target_compile_options(thermal PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(thermal PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
