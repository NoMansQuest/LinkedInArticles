/*
 * MIT License
 *
 * Copyright (c) 2026 Nasser Ghoseiri

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <new> // Required for interference_size constants


#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_destructive_interference_size;
    const size_t CACHE_LINE_SIZE = hardware_destructive_interference_size;
#else
    // Fallback for older compilers that don't support the feature yet
    const size_t CACHE_LINE_SIZE = 64;
#endif

const long long ITERATIONS = 100'000'000;

// SCENARIO 1: High Contention (False Sharing)
struct SharedData {
    std::atomic<long long> countA{0}; // Offset 0
    std::atomic<long long> countB{0}; // Offset 8 - Lives on the same 64-byte line!
};

// SCENARIO 2: Fixed (Hardware Aware)
struct AlignedData {
    alignas(CACHE_LINE_SIZE) std::atomic<long long> countA{0};
    alignas(CACHE_LINE_SIZE) std::atomic<long long> countB{0}; 
};

template <typename T>
void benchmark(const std::string& label, T& data) {
    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1([&]() 
    {
        for (long long i = 0; i < ITERATIONS; ++i)
            data.countA++;
    });

    std::thread t2([&]() 
    {
        for (long long i = 0; i < ITERATIONS; ++i)
            data.countB++;
    });

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << label << ": " << elapsed.count() << " seconds\n";
}

int main() {
    SharedData problematic;
    AlignedData fixed;

    std::cout << "Running Benchmarks (" << ITERATIONS << " iterations)...\n" << std::endl;

    // Run a few times to warm up the cache
    benchmark("Scenario 1: False Sharing (Contention)", problematic);
    benchmark("Scenario 2: Aligned (Independent)", fixed);

    return 0;
}