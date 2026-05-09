// =============================================================================
// bench_cpp11.cpp  -  Parallel reduction, C++11 style (std::thread)
//
// Compile:  g++ -O2 -std=c++11 -o bench_cpp11 bench_cpp11.cpp
// Run:      ./bench_cpp11
//
// Mechanism: same manual chunk/thread/join pattern as C++98.
//
// What changed vs C++98 - ergonomics and portability, not performance:
//   + std::thread   - portable across Windows/Linux/macOS; no -lpthread flag,
//                     no #ifdef for Win32 vs POSIX
//   + Lambdas       - worker logic lives inline; no ThreadArg struct, no void*
//                     casts, no separate function definition
//   + std::vector   - dynamic thread/result storage; no fixed-size arrays
//   + <chrono>      - portable high-resolution timer; no clock_gettime
//   + emplace_back  - construct thread in-place; no copy/move issues
//
//   - Runtime cost: identical to C++98 (same OS threads underneath)
//   - Still manual: must call join() explicitly; forgetting is undefined behaviour
//     (fixed in C++20 with std::jthread)
//
// The algorithm is unchanged. The infrastructure shrank from ~30 lines to ~10.
// =============================================================================
 
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <numeric>
 
static const std::size_t N        = 10000000;
static const int         NTHREADS = 8;
 
static std::tuple<double, double> run(const std::vector<double>& data)
{
    std::vector<double>      partials(NTHREADS, 0.0);
    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);
 
    std::size_t chunk = N / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
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
        th.join();
    }
 
    auto total = std::accumulate(partials.cbegin(), partials.cend(), 0.0f);     
    auto latency = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, latency);
}
 
int main()
{
    std::vector<double> data(N);
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);
 
    run(data);                          // warm-up
    auto benchmark = run(data);         // measured
 
    std::printf("[C++11 / std::thread] total: %.4f,  time = %.4f ms   threads = %d\n",
                std::get<0>(benchmark), std::get<1>(benchmark), NTHREADS);
    return 0;
}