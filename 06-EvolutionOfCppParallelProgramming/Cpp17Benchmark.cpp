// =============================================================================
// bench_cpp17.cpp  —  Parallel reduction, C++17 style (parallel STL)
//
// Compile (GCC + TBB):   g++ -O2 -std=c++17 -ltbb -o bench_cpp17 bench_cpp17.cpp
// Compile (LLVM libc++): clang++ -O2 -std=c++17 -fexperimental-library \
//                                -o bench_cpp17 bench_cpp17.cpp
// Run:  ./bench_cpp17
//
// What's new — the BIG shift:
//   * std::reduce with std::execution::par_unseq
//     The programmer NO LONGER writes threads, chunks, or joins.
//     The STL implementation (backed by TBB, OpenMP, or a vendor runtime)
//     manages the thread pool, work-stealing, and hardware topology.
//
//   * Execution policies (§25.3 [execpol]):
//       std::execution::seq          - serial (baseline)
//       std::execution::par          - multi-threaded, ordered
//       std::execution::par_unseq    - multi-threaded + SIMD
//
//   * std::reduce vs std::accumulate:
//       accumulate  - strictly left-to-right (serial); cannot parallelise.
//       reduce      - associative & commutative; safe to parallelise with FP.
//
// Trade-offs:
//   + Zero boilerplate threading code — fewer bugs, no forgotten join().
//   + Implementation can use SIMD + thread pools unavailable to raw threads.
//   - Requires linking TBB (or equivalent); not header-only.
//   - First call may pay thread-pool initialisation cost (~1–5 ms).
//   - For tiny N, par_unseq is slower than seq due to scheduling overhead.
// =============================================================================
 
#include <algorithm>
#include <execution>    // C++17 parallel execution policies
#include <numeric>      // std::reduce
#include <vector>
#include <chrono>
#include <cstdio>
 
static const std::size_t N = 5000000;
 
// Helper: time one run of the given execution policy
template <typename Policy>
std::tuple<double, double> bench(Policy policy, const std::vector<double>& data)
{
    auto t0 = std::chrono::steady_clock::now(); 
    auto total = std::reduce(policy, data.begin(), data.end(), 0.0f); // identity element 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();     
    return std::make_tuple(total, elapsed);
}
 
int main()
{
    std::vector<double> data(N);
    // std::iota would give integers; we want the harmonic series
    for (std::size_t i = 0; i < N; ++i)
    {
        data[i] = 1.0 / (i + 1.0);
    }
 
    // Run all three policies so the overhead difference is visible
    auto t1 = bench(std::execution::seq, data);
    auto t2 = bench(std::execution::par, data);
    auto t3 = bench(std::execution::par_unseq, data);

    std::printf("[C++17 / seq]  total = %.6f   time = %.4f ms\n", std::get<0>(t1), std::get<1>(t1));
    std::printf("[C++17 / par]  total = %.6f   time = %.4f ms\n", std::get<0>(t2), std::get<1>(t2));
    std::printf("[C++17 / par_unseq]  total = %.6f   time = %.4f ms\n", std::get<0>(t3), std::get<1>(t3)); 
    return 0;
}