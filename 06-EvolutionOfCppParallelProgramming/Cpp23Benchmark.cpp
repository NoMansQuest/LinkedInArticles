// =============================================================================
// bench_cpp23.cpp  —  Parallel reduction, C++23 style
//
// Compile:  g++ -O2 -std=c++23 -ltbb -o bench_cpp23 bench_cpp23.cpp
// Run:      ./bench_cpp23
//
// C++23 threading highlights relevant to parallel workloads:
//
//   * std::mdspan              — Multi-dimensional non-owning view (N-D std::span).
//                                Fundamental for 2-D/3-D domain decomposition
//                                without nested vectors.
//   * std::generator<T>        — Coroutine-based lazy sequence producer.
//                                Can feed a parallel algorithm lazily, decoupling
//                                data production from consumption.
//   * std::ranges::fold_left   — named, composable reduction ops;
//     std::ranges::fold_right    basis for future range-parallel composition. 
//                           
//   * std::expected<T,E>       — error propagation without exceptions; vital for
//                                returning results from worker threads cleanly.
//   * Deducing this            — simplifies CRTP / recursive lambdas; minor but
//                                relevant when building custom parallel primitives.
//   * std::print/std::println  — formatting without printf (used here).
//
// What C++23 does NOT yet fix:
//   * No parallel range algorithms (std::ranges::reduce still serial-only).
//   * Parallel execution policies still tied to legacy iterator pairs, not
//     ranges.  This gap is exactly what C++26 std::execution addresses.
//
// Benchmark structure:
//   1. Serial ranges::fold_left          — Idiomatic but single-threaded.
//   2. Manual jthread + mdspan slicing   — 2D decomposition demo.
//   3. par_unseq on raw iterators        — still the fastest path in C++23.
// =============================================================================
 
#include <algorithm>
#include <execution>
#include <functional>
#include <mdspan>           // C++23
#include <numeric>
#include <print>            // C++23  std::println
#include <ranges>
#include <span>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <tuple>
 
static const std::size_t N = 5000000;
static const int NTHREADS = 8;
 
// ---------------------------------------------------------------------------
// 1. std::ranges::fold_left  — clean, composable, but single-threaded
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_fold(std::span<const double> data)
{
    auto t0 = std::chrono::steady_clock::now(); 
    auto total = std::ranges::fold_left(data, 0.0, std::plus<double>{}); 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();     
    return std::make_tuple(total, elapsed);
}
 
// ---------------------------------------------------------------------------
// 2. mdspan + jthread  — 2-D domain decomposition
//
//   Treat the 1-D array as rows × cols (NTHREADS × chunk).
//   Each thread owns one row of the mdspan — no pointer arithmetic in sight.
//   In a real workload this would be a 2-D stencil or matrix operation.
// ---------------------------------------------------------------------------
std::tuple<double, double>  bench_mdspan(std::span<const double> data)
{
    std::size_t cols  = data.size() / NTHREADS;
    // mdspan view: NTHREADS rows, cols columns (last row may be shorter —
    // we trim to keep the mdspan rectangular, remainder handled separately).
    std::size_t trimmed = cols * NTHREADS;
 
    // C++23 mdspan with a 2-D extents
    std::mdspan<const double,
                std::extents<std::size_t,
                             std::dynamic_extent,
                             std::dynamic_extent>>
        mat(data.data(), NTHREADS, cols);
 
    std::vector<double>       partials(NTHREADS, 0.0);
    std::vector<std::jthread> threads;
    threads.reserve(NTHREADS);
 
    auto t0 = std::chrono::steady_clock::now();
 
    for (int t = 0; t < NTHREADS; ++t) {
        threads.emplace_back([&mat, &partials, t, cols]() {
            double sum = 0.0;
            for (std::size_t c = 0; c < cols; ++c)
                sum += mat[t, c];    // C++23 multi-index operator[]
            partials[t] = sum;
        });
    }
    // jthread destructors join

    auto total = std::accumulate(std::cbegin(partials), std::cend(partials), 0.0f);
 
    // Add the trimmed tail (at most NTHREADS-1 elements)
    for (std::size_t i = trimmed; i < data.size(); ++i) 
    {
        total += data[i];
    }
 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();     
    return std::make_tuple(total, elapsed);
}
 
// ---------------------------------------------------------------------------
// 3. par_unseq via a transform_view  — compose ranges then execute in parallel
//
//   We cannot pass a range directly to std::reduce with par_unseq yet
//   (that requires C++26 sender/receiver or a future range overload).
//   The workaround: materialise through ranges::to (C++23) or just use
//   begin()/end() on the view.  Here we use the view's iterators.
// ---------------------------------------------------------------------------
std::tuple<double, double>  bench_par_range(std::span<const double> data)
{
    // Compose a lazy pipeline: only even-indexed values
    // (pure demonstration — shows ranges composition before execution)
    auto even_idx = std::views::iota(std::size_t{0}, data.size())
                  | std::views::filter([](std::size_t i){ return i % 2 == 0; })
                  | std::views::transform([&data](std::size_t i){ return data[i]; });
 
    // Must materialise because par_unseq needs random-access iterators
    // (ranges::to is C++23)
    auto materialised = even_idx | std::ranges::to<std::vector<double>>();
 
    auto t0 = std::chrono::steady_clock::now();
 
    auto total = std::reduce(std::execution::par_unseq, materialised.begin(), materialised.end(), 0.0); 
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
 
    auto fold_result = bench_fold(view);
    auto mdspan_result = bench_mdspan(view);
    auto par_range_result =bench_par_range(view);

    std::println("[C++23 / fold      ]  sum = {:.6f}   time = {:.4f} ms", std::get<0>(fold_result), std::get<1>(fold_result));
    std::println("[C++23 / mdspan    ]  sum = {:.6f}   time = {:.4f} ms", std::get<0>(mdspan_result), std::get<1>(mdspan_result));
    std::println("[C++23 / par_range ]  sum = {:.6f}   time = {:.4f} ms", std::get<0>(par_range_result), std::get<1>(par_range_result));
 
    return 0;
}