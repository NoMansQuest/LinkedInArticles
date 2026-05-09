// =============================================================================
// bench_cpp11.cpp  —  Parallel reduction, C++11 style (std::thread)
//
// Compile:  g++ -O2 -std=c++11 -o bench_cpp11 bench_cpp11.cpp
// Run:      ./bench_cpp11
//
// What's new vs C++98:
//   - std::thread  - portable, RAII-friendly thread handle
//   - Lambdas      - worker logic lives inline, no helper struct needed
//   - std::vector  - dynamic thread / result storage without raw arrays
//   - auto, range-for, <chrono> - modern ergonomics
//   - std::atomic  - shown in comment; not needed here because we join
//                    before reading results (happens-before guarantee).
//
// Penalty vs C++98: essentially none for this pattern; the OS thread cost
//   is identical.  The gain is purely in readability and safety.
// =============================================================================
 
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstdio>
 
static const std::size_t N = 5000000;
static const int NTHREADS = 8;
 
int main()
{
    std::vector<double> data(N);
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);
 
    std::vector<double>      partials(NTHREADS, 0.0);
    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);
 
    std::size_t chunk = N / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    // Lambda captures the slice by index; result stored in partials[t]
    for (int t = 0; t < NTHREADS; ++t) {
        std::size_t begin = t * chunk;
        std::size_t end   = (t == NTHREADS - 1) ? N : begin + chunk;
 
        threads.emplace_back([&data, &partials, t, begin, end]() {
            double sum = 0.0;
            for (std::size_t i = begin; i < end; ++i)
                sum += data[i];
            partials[t] = sum;
        });
    }
 
    for (auto& th : threads)
    {
        // join() provides the happens-before; safe to read partials.
        // NOTE: C++17 introduced std::jthread, which would no longer need
        //       manual joining before they get de-scoped.
        th.join();   
    }
 
    double total = std::accumulate(partials.cbegin(), partials.cend(), 0.0f); 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); 
    std::printf("[C++11 / std::thread]  sum = %.6f   time = %.4f ms   threads = %d\n", total, elapsed, NTHREADS);
    return 0;
}