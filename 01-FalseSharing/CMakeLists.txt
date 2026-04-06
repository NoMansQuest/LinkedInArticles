cmake_minimum_required(VERSION 3.15)
project(FalseSharingDemo LANGUAGES CXX)

# Ensure C++17 for std::hardware_destructive_interference_size
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(false_sharing_demo main.cpp)

# Find the Threads package (Crucial for std::thread and timing synchronization)
find_package(Threads REQUIRED)

# Link Threads to your executable
target_link_libraries(false_sharing_demo PRIVATE Threads::Threads)

# Compiler Specific Flags
if(MSVC)
    target_compile_options(false_sharing_demo PRIVATE /W4 /O2)
else()
    # -pthread is usually handled by Threads::Threads, but we add 
    # optimizations to ensure the benchmark isn't "too fast" to measure
    target_compile_options(false_sharing_demo PRIVATE -Wall -Wextra -O3 -march=native)
endif()