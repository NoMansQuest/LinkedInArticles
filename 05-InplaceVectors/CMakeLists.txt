cmake_minimum_required(VERSION 3.15)

# We define the Project Name and Version
project(Cpp26InplaceVectors VERSION 1.0 LANGUAGES CXX)

# Inplace Vectors was introduced in C++26, hence we need it
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define our executable
add_executable(inplace_vectors_demo main.cpp)

# Compiler Specific Optimization Flags
if(MSVC)
    # High optimization for Windows/Visual Studio
    target_compile_options(inplace_vectors_demo PRIVATE /W4 /O2 /permissive-)
else()
    # High optimization for GCC/Clang (Linux/macOS)
    target_compile_options(inplace_vectors_demo PRIVATE -Wall -Wextra -O2 -march=native)
endif()

# Threading Support (Since PMR often targets multi-threaded apps)
find_package(Threads REQUIRED)
target_link_libraries(inplace_vectors_demo PRIVATE Threads::Threads)