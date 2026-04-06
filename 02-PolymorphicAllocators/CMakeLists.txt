cmake_minimum_required(VERSION 3.15)

# We define the Project Name and Version
project(Polymorphic_Allocators VERSION 1.0 LANGUAGES CXX)

# Since we're using std::ranges and std::views, we need a minimum C++20 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define our executable
add_executable(pmr_demo main.cpp)

# Compiler Specific Optimization Flags
if(MSVC)
    # High optimization for Windows/Visual Studio
    target_compile_options(pmr_demo PRIVATE /W4 /O2 /permissive-)
else()
    # High optimization for GCC/Clang (Linux/macOS)
    target_compile_options(pmr_demo PRIVATE -Wall -Wextra -O3 -march=native)
endif()

# Threading Support (Since PMR often targets multi-threaded apps)
find_package(Threads REQUIRED)
target_link_libraries(pmr_demo PRIVATE Threads::Threads)