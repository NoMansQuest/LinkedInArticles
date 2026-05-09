// =============================================================================
// bench_cpp14.cpp  —  Parallel reduction, C++14 style
//
// Compile:  g++ -O2 -std=c++14 -o bench_cpp14 bench_cpp14.cpp
// Run:      ./bench_cpp14
//
// What's new vs C++11:
//   * Generic lambdas (auto parameters) - the worker lambda is now a
//     reusable template without needing std::function overhead.
//   * std::make_unique - safe heap allocation without raw new/delete.
//   * Return-type deduction (auto fn() -> ...) - cleaner helper signatures.
//   * _t / _v aliases (e.g. std::decay_t) - shorter type-trait spellings.
//
// In practice C++14 is a "C++11 done right" release; threading primitives
// are unchanged, but the surrounding code becomes considerably tighter.
//
// Penalty/Benefit vs C++11: identical runtime; zero overhead from language
//   features.  Benefits are ergonomic — easier to write correct code.
// =============================================================================
 
#include <thread>
#include <vector>
#include <memory>       // std::make_unique
#include <chrono>
#include <cstdio>
 
static const std::size_t N = 5000000;
static const int NTHREADS = 8;
 
// ---------------------------------------------------------------------------
// C++14 generic lambda: 'auto' parameter infers the iterator/pointer type.
// This would require a template function or std::function in C++11.
// ---------------------------------------------------------------------------
auto make_reducer = [](const double* first, const double* last) -> double {
    double sum = 0.0;
    for (; first != last; ++first)
    {
        sum += *first;
    }
    return sum;
};
 
int main()
{
    // std::make_unique — the "correct" way since C++14 (exception-safe)
    auto data = std::make_unique<double[]>(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        data[i] = 1.0 / (i + 1.0);
    }
 
    std::vector<double>      partials(NTHREADS, 0.0);
    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);
 
    std::size_t chunk = N / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    for (int t = 0; t < NTHREADS; ++t) 
    {
        const double* begin = data.get() + t * chunk;
        const double* end   = (t == NTHREADS - 1) ? data.get() + N : begin + chunk;
        // Capture pointers by value — no aliasing issues with unique_ptr
        threads.emplace_back([begin, end, t, &partials]() 
        {
            partials[t] = make_reducer(begin, end);
        });
    }
 
    for (auto& th : threads)
    {
        // NOTE: If you leave any std::thread un-joined, you will get an exception on runtime
        //       and your application will crash. C++17 introduced std::jthread which no longer
        //       required manual joining.
        th.join();
    }
 
    double total = 0.0;
    for (double p : partials)
    {
        total += p;
    }
 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); 
    std::printf("[C++14 / std::thread]  sum = %.6f   time = %.4f ms   threads = %d\n", total, elapsed, NTHREADS);
    return 0;
}