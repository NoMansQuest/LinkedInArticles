// =============================================================================
// bench_cpp17.cpp  -  Parallel reduction, C++17 style (parallel STL)
//
// Compile (GCC + TBB):  g++ -O2 -std=c++17 -ltbb -o bench_cpp17 bench_cpp17.cpp
// Run:                  ./bench_cpp17
//
// Mechanism: std::reduce with std::execution::par_unseq.
//
// What changed vs C++11 - the first real performance shift:
//   + No threading code in user space at all: no spawn, no chunk, no join
//   + par_unseq = multi-threading AND SIMD vectorisation simultaneously
//   + The runtime (TBB/vendor) handles work-stealing, load balancing,
//     NUMA placement, and hardware topology - things manual thread code
//     typically ignores entirely
//   + std::reduce (vs std::accumulate): associative + commutative contract
//     allows reordering operations, which is what makes parallelisation safe
//
//   - Must link TBB or a vendor parallel STL backend
//   - Thread pool cold-start on first call (~15-40 ms); this file warms up
//     with one unmeasured call before timing
//   - par_unseq requires the operation to be vectorisation-safe:
//     no locks, no exceptions thrown inside the reduction
//   - C++14/20/23 added nothing to parallel algorithms; par_unseq introduced
//     here remains the idiomatic CPU parallel reduction through C++23
//
// Code size vs C++11: the threading infrastructure dropped to a single line.
// The complexity didn't disappear - it moved inside the library.
// C++26 makes that internal structure visible and composable again.
// =============================================================================
 
#include <execution>
#include <numeric>
#include <vector>
#include <chrono>
#include <cstdio>
#include <tuple>
 
static const std::size_t N = 10000000;
 
static std::tuple<double, double> run(const std::vector<double>& data)
{
    auto t0 = std::chrono::steady_clock::now();
 
    double total = std::reduce(
        std::execution::par_unseq,
        data.begin(), data.end(),
        0.0);
 
    auto duration = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, duration);
}
 
int main()
{
    std::vector<double> data(N);
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);
 
    run(data);                          // warm-up (pays thread pool cold-start)
    auto benchmark = run(data);         // measured
 
    std::printf("[C++17 / par_unseq] total: %.4f, time = %.4f ms\n", std::get<0>(benchmark), std::get<1>(benchmark));
    return 0;
}