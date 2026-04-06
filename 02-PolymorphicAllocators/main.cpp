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

#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <memory_resource>
#include <array>
#include <ranges>


// Non-optimized generic approach.
void naive_worker(int thread_id, int num_jobs) 
{
    // No heap-allocated buffer, just allocate as you go forward...
    for (int job = 0; job < num_jobs; ++job) 
    {
        std::vector<std::string> strings; // Standard STL vector, uses global-heap
        std::vector<int> numbers; // Standard STL vector, uses global-heap

        for (int i = 0; i < 25; ++i) 
        {
            auto content = "Packet data " + std::to_string(i) + " thread " + std::to_string(thread_id);
            strings.emplace_back(content);
            numbers.assign(35, thread_id + i);   // small vector allocation + initialization
        }
    }
}

// Optimized worker, using memory_resources introduced in C++17!
void pmr_worker(int thread_id, int num_jobs)
{
    // Use heap-allocated buffer
    constexpr size_t arena_size = 2 * 1024 * 1024;  // 2 MB per thread
    auto buffer = std::make_unique<std::byte[]>(arena_size);

    // This is our optimization, we're using polymorphic allocator, part of
    // memory_resources (i.e. std::mpr) introduced in C++17 (though seldome used!)
    std::pmr::monotonic_buffer_resource mbr(buffer.get(), arena_size);
    std::pmr::polymorphic_allocator<char> alloc(&mbr);

    for (int job = 0; job < num_jobs; ++job)
    {
        // Likewise, our vector is coming from "std::pmr" as well.
        std::pmr::vector<std::pmr::string> strings(&mbr); // Uses our arena, NOT the global heap
        std::pmr::vector<int> numbers(&mbr); 
        strings.reserve(25);

        for (int i = 0; i < 25; ++i)
        {
            std::pmr::string s("Packet data " + std::to_string(i) + " thread " + std::to_string(thread_id), alloc);
            strings.push_back(std::move(s));
            numbers.assign(35, thread_id + i);
        }
    }
}

/// <summary>
///     Main function here runs two sets of benchmarks, one run invoking 'naive_worker' and the 
///     other using 'pmr_worker'. We measure the total time each run takes and report it back to
///     the std::cout.
/// </summary>
int main() 
{
    const int num_threads = std::thread::hardware_concurrency(); // Adjusted to our core count
    const int jobs_per_thread = 20000; // A rather big number to stress-test memory fragmentation

    std::vector<std::thread> threads;
    std::chrono::steady_clock::time_point start, end;
    std::chrono::milliseconds duration;

    // ------------ Run the non-optimized naive version
    std::cout << "Naive version: Running with " << num_threads << " threads...\n";
    start = std::chrono::high_resolution_clock::now();

    for (auto i : std::views::iota(0, num_threads))
    {
        threads.emplace_back(naive_worker, i, jobs_per_thread);
    }

    for (auto& t : threads) 
    {
        t.join();
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Naive version took: " << duration.count() << " ms\n";

    // ------------- Run the optimized version
    threads.clear();
    std::cout << "PMR arena version: Running with " << num_threads << " threads...\n";
    start = std::chrono::high_resolution_clock::now();
    
    for (auto i : std::views::iota(0, num_threads))
    {
        threads.emplace_back(pmr_worker, i, jobs_per_thread);
    }

    for (auto& t : threads) 
    {
        t.join();
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "PMR arena version took: " << duration.count() << " ms\n";

    // And we're done...
    return 0;
}
