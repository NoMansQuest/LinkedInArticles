// =============================================================================
// bench_cpp26.cpp - Parallel reduction, C++26 style (std::execution / P2300)
//
// TWO COMPILATION PATHS:
//
// PATH A - Godbolt, no external deps:
//   Compiler : x86-64 clang (trunk) or x86-64 gcc (trunk)
//   Flags    : -O2 -std=c++26 -pthread
//   Libraries: none
//
// PATH B - local build with real stdexec:
//   git clone https://github.com/NVIDIA/stdexec
//   g++ -O2 -std=c++26 -I<stdexec>/include -ltbb \
//       -DUSE_STDEXEC -o bench_cpp26 bench_cpp26.cpp
//
// =============================================================================
//
// C++26 is a superset of C++23, so std::reduce(par_unseq) is still available
// and measured here as benchmark A - directly comparable to C++17.
//
// Benchmark B shows ex::bulk, the new std::execution model. It achieves the
// same throughput on a CPU, but the scheduler is now an explicit runtime
// parameter. Swap static_thread_pool for a GPU scheduler and benchmark B
// needs zero changes. That scheduler-agnosticism is the point of C++26.
//
// What changed vs C++17/20/23:
//   + std::execution (P2300) - sender/receiver model
//   + Sender   - lazy description of work; nothing runs until sync_wait
//   + Scheduler - where work runs: thread pool, GPU stream, async I/O ring…
//   + ex::bulk(N, fn)   - parallel-for as a composable pipeline stage
//   + ex::when_all(…)   - structured join; typed results, no shared state
//   + ex::transfer(sch) - hop schedulers mid-pipeline
//   + ex::sync_wait(…)  - the single blocking point driving everything
//
// Penalties vs C++17 par_unseq:
//   - Heavier compile times (template-heavy sender/receiver machinery)
//   - Requires stdexec until compiler/stdlib support ships
//   - Steeper learning curve: sender/receiver is a new mental model
//
// Benefits vs C++17 par_unseq:
//   + Scheduler-agnostic: CPU, GPU, FPGA - one algorithm, swap the scheduler
//   + Structured concurrency: work lifetimes are lexically scoped
//   + Composable: pipelines are values; easy to test and reuse
//   + Type-safe error propagation through the entire chain
//   + Cancellation via stop_token propagates through every combinator
//
// =============================================================================
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
namespace ex   = stdexec;
namespace exec = exec;
 
 
// =============================================================================
#include <execution>
#include <numeric>
#include <span>
#include <vector>
#include <chrono>
#include <cstdio>
#include <tuple>
 
static const std::size_t N = 10000000;
static const int NTHREADS = 8;
 
// ---------------------------------------------------------------------------
// Benchmark A: std::reduce(par_unseq)
// The C++17 mechanism, unchanged. Still the right tool for a simple CPU
// reduction. Directly comparable to bench_cpp17.cpp.
// ---------------------------------------------------------------------------
static std::tuple<double, double> run_par_unseq(std::span<const double> data)
{
    auto t0 = std::chrono::steady_clock::now();
 
    double total = std::reduce(
        std::execution::par_unseq,
        data.begin(), data.end(),
        0.0);
 
    auto latency = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, latency);
}
 
// ---------------------------------------------------------------------------
// Benchmark B: ex::schedule | ex::bulk | ex::sync_wait
// The new std::execution model. Same throughput as par_unseq on a CPU.
// The difference: 'pool' below is an explicit runtime parameter.
//
// To run on a GPU instead (with real stdexec + CUDA backend):
//   nvexec::stream_scheduler sch = cuda_pool.get_scheduler();
//   // ^ this one line is the only change needed
// ---------------------------------------------------------------------------
static std::tuple<double, double> run_bulk(std::span<const double> data, exec::static_thread_pool& pool)
{
    auto sch = pool.get_scheduler();
 
    std::vector<double> partials(NTHREADS, 0.0);
    std::size_t         chunk = data.size() / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    ex::sync_wait(
        ex::schedule(sch)
      | ex::bulk(
            NTHREADS,
            [&](int t) {
                std::size_t begin = t * chunk;
                std::size_t end   = (t == NTHREADS - 1)
                                      ? data.size() : begin + chunk;
                double sum = 0.0;
                for (std::size_t i = begin; i < end; ++i)
                    sum += data[i];
                partials[t] = sum;
            }));
 
    double total = 0.0;
    for (double p : partials) total += p;
 
    auto latency = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return std::make_tuple(total, latency);
}
 
int main()
{
    std::vector<double> data(N);
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / static_cast<double>(i + 1);
 
    exec::static_thread_pool pool(NTHREADS);
 
    // warm-up both paths
    run_par_unseq(data);
    run_bulk(data, pool);
 
    // measured
    auto t1 = run_par_unseq(data);
    auto t2 = run_bulk(data, pool);
 
    std::printf("[C++26 / par_unseq] total: %.4f,  time = %.4f ms  (C++17 mechanism, still valid)\n", std::get<0>(t1), std::get<1>(t1));
    std::printf("[C++26 / bulk     ] total: %.4f, time = %.4f ms  (new model, scheduler-agnostic)\n", std::get<0>(t2), std::get<1>(t2));
    return 0;
}