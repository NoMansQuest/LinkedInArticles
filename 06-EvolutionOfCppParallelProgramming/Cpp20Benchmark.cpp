// =============================================================================
// bench_cpp20.cpp  —  Parallel reduction, C++20 style
//
// Compile:  g++ -O2 -std=c++20 -ltbb -o bench_cpp20 bench_cpp20.cpp
// Run:      ./bench_cpp20
//
// C++20 does NOT add new parallel algorithms beyond C++17's execution
// policies.  Its threading contributions are ergonomic and safety-oriented:
//
//   • std::jthread  — joins automatically on destruction (RAII join).
//                     Also supports cooperative cancellation via stop_token.
//   • std::barrier  — reusable synchronisation point for fork/join loops.
//   • std::latch    — single-use countdown for "wait for N threads to finish".
//   • std::stop_source / std::stop_token — cancellation without shared atomics.
//   • Coroutines    — co_await / co_yield (generators, async I/O); not yet
//                     useful for CPU-bound parallel reduction, but they
//                     underpin future async executors.
//   • std::atomic<std::shared_ptr<T>>  — lock-free smart-pointer update.
//   • std::span     — non-owning, bounds-safe view over contiguous data;
//                     perfect for describing per-thread slices.
//
// This benchmark shows all three synchronisation primitives so you can see
// the ergonomic progression.  Runtime is essentially identical to C++17 par.
//
// Penalty/Benefit vs C++17:
//   *  std::jthread: can't forget to join() — no resource leaks.
//   *  std::barrier: replaces manual condition_variable choreography.
//   *  std::span: zero-overhead slice abstraction; compiler can optimise.
//   *  jthread's stop_token machinery adds ~a few bytes per thread object.
// =============================================================================
 
#include <thread>
#include <barrier>
#include <latch>
#include <span>
#include <vector>
#include <numeric>
#include <execution>
#include <chrono>
#include <cstdio>
 
static const std::size_t N        = 5000000;
static const int         NTHREADS = 8;
 
// ---------------------------------------------------------------------------
// Demo 1: std::jthread  — automatic RAII join, no explicit join() needed
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_jthread(std::span<const double> data)
{
    std::vector<double>       partials(NTHREADS, 0.0);
    std::vector<std::jthread> threads;          // jthread, not thread
    threads.reserve(NTHREADS);
 
    std::size_t chunk = data.size() / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    for (int t = 0; t < NTHREADS; ++t) {
        std::size_t begin = t * chunk;
        std::size_t end   = (t == NTHREADS - 1) ? data.size() : begin + chunk;
 
        // std::span slice — no pointer arithmetic in user code
        auto slice = data.subspan(begin, end - begin); 
        threads.emplace_back([slice, t, &partials](std::stop_token /*stoken*/) {
            // stop_token can be polled to cancel long work gracefully
            double sum = 0.0;
            for (double v : slice)
                sum += v;
            partials[t] = sum;
        });
    }
    // Destructor of jthread calls join() — threads finish here automatically
 
    auto total = std::accumulate(std::cbegin(partials), std::cend(partials), 0.0f);
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, elapsed);
}
 
// ---------------------------------------------------------------------------
// Demo 2: std::barrier  — reusable fork/join without condition_variable
//
// Useful when you want to run MULTIPLE phases with the same thread pool
// (e.g., reduce → transform → reduce again) without respawning threads.
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_barrier(std::span<const double> data)
{
    std::vector<double> partials(NTHREADS, 0.0);
    double              total   = 0.0;
    bool                running = true;
 
    // Completion callback runs once all threads hit barrier.arrive_and_wait()
    auto on_completion = [&]() noexcept {
        total = std::accumulate(std::cbegin(partials), std::cend(partials), 0.0f);
        // Could kick off a second phase here (transform, etc.)
        running = false;    // signal threads to exit after barrier lifts
    };
 
    std::barrier sync(NTHREADS, on_completion); 
    std::size_t chunk = data.size() / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();

    {
        std::vector<std::jthread> threads;
        threads.reserve(NTHREADS);
 
        for (int t = 0; t < NTHREADS; ++t) {
            auto slice = data.subspan(t * chunk,
                (t == NTHREADS - 1) ? data.size() - t * chunk : chunk);
 
            threads.emplace_back([slice, t, &partials, &sync]() {
                double sum = 0.0;
                for (double v : slice) sum += v;
                partials[t] = sum;
                sync.arrive_and_wait();   // wait for all; then callback fires
            });
        }
        // jthread destructors join here, given the end-of-scope
    }
     
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, elapsed);
}
 
// ---------------------------------------------------------------------------
// Demo 3: C++17 parallel STL still available and still the fastest one-liner
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_par_unseq(std::span<const double> data)
{
    auto t0 = std::chrono::steady_clock::now();
    auto total = std::reduce(std::execution::par_unseq, data.begin(), data.end(), 0.0f);
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, elapsed);
}
 
int main()
{
    std::vector<double> data(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        data[i] = 1.0 / (i + 1.0);
    }
 
    std::span<const double> view(data);
 
    auto t1 = bench_jthread(view);
    auto t2 = bench_barrier(view);
    auto t3 = bench_par_unseq(view);
 
    std::printf("[C++20 / jthread ]  total = %.6f, time = %.4f ms\n", std::get<0>(t1), std::get<1>(t1));
    std::printf("[C++20 / barrier ]  total = %.6f, time = %.4f ms\n", std::get<0>(t2), std::get<1>(t2));
    std::printf("[C++20 / par_unseq] total = %.6f, time = %.4f ms\n", std::get<0>(t3), std::get<1>(t3)); 
    return 0;
}