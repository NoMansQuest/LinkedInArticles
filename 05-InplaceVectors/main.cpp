#include <iostream>
#include <vector>
#include <inplace_vector> // C++26
#include <boost/container/static_vector.hpp>
#include <chrono>

// Count allocations to prove stack-only behavior
static size_t g_allocations = 0;
void* operator new(size_t size) {
    g_allocations++;
    return std::malloc(size);
}

const int ITERATIONS = 500'000;
const int CAPACITY = 32;

template <typename Func>
void run_test(std::string_view name, Func func) {
    g_allocations = 0;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        func();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;
    
    std::cout << name << " | Time: " << ms.count() << "ms | Heap Allocs: " << g_allocations << "\n";
}

int main() {
    std::cout << "Benchmarking " << ITERATIONS << " runs (Capacity " << CAPACITY << ")\n";
    std::cout << "---------------------------------------------------------------\n";

    // 1. Standard Vector (Naive)
    run_test("std::vector (Naive)  ", []() {
        std::vector<int> v;
        for(int i=0; i<CAPACITY; ++i) v.push_back(i);
    });

    // 2. Standard Vector (Reserved)
    run_test("std::vector (Reserve)", []() {
        std::vector<int> v;
        v.reserve(CAPACITY);
        for(int i=0; i<CAPACITY; ++i) v.push_back(i);
    });

    // 3. Boost Static Vector
    run_test("boost::static_vector ", []() {
        boost::container::static_vector<int, CAPACITY> v;
        for(int i=0; i<CAPACITY; ++i) v.push_back(i);
    });

    // 4. C++26 In-Place Vector (unsafe push_back)
    run_test("std::inplace_vector (unsafe) ", []() {
        std::inplace_vector<int, CAPACITY> v;
        for(int i=0; i<CAPACITY; ++i) v.unchecked_push_back(i);
    });

    // 5. C++26 In-Place Vector (safe push_back)
    run_test("std::inplace_vector (safe) ", []() {
        std::inplace_vector<int, CAPACITY> v;
        for(int i=0; i<CAPACITY; ++i) v.push_back(i);
    });

    return 0;
}