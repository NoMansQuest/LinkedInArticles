
#include <stdexec/execution.hpp>         // P2300 reference implementation
#include <exec/static_thread_pool.hpp>   // bundled with stdexec
 
#include <vector>
#include <span>
#include <numeric>
#include <chrono>
#include <cstdio>
#include <optional>
#include <tuple> 

static const std::size_t N        = 5000000;
static const int NTHREADS = 8;
 
// ---------------------------------------------------------------------------
// Benchmark 1: stdexec::bulk  — the direct C++26 parallel-for replacement
//
//   stdexec::bulk(sender, shape, fn) fires 'shape' tasks, each receiving an index.
//   The scheduler decides how many threads to use; we just declare intent.
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_bulk(std::span<const double> data, exec::static_thread_pool& pool)
{
    auto sch = pool.get_scheduler();
 
    std::vector<double> partials(NTHREADS, 0.0);
    std::size_t         chunk = data.size() / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    // Build the sender pipeline — nothing runs yet
    auto work =
        stdexec::schedule(sch)                          // start on thread pool
      | stdexec::bulk(                                   // fan-out NTHREADS tasks
            NTHREADS,
            [&](int t) {                           // called once per task index
                std::size_t begin = t * chunk;
                std::size_t end   = (t == NTHREADS - 1)
                                      ? data.size()
                                      : begin + chunk;
                double sum = 0.0;
                for (std::size_t i = begin; i < end; ++i)
                    sum += data[i];
                partials[t] = sum;                 // written by one task only
            });
 
    stdexec::sync_wait(std::move(work));                // blocks; runs everything
 
    auto total = std::accumulate(std::cbegin(partials), std::cend(partials), 0);
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); 
    return std::make_tuple(total, elapsed);
}
 
// ---------------------------------------------------------------------------
// Benchmark 2: stdexec::when_all  — independent senders joined structurally
//
//   Each thread slice becomes its own typed sender.  when_all joins them and
//   forwards all results as a tuple — no shared vector, no races.
//   This is the "pure" sender/receiver pattern with no shared mutable state.
//
//   Note: with NTHREADS=8 this creates 8 senders; in real code you'd generate
//   them with a fold or std::apply over a parameter pack.
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_when_all(std::span<const double> data, exec::static_thread_pool& pool)
{
    auto sch = pool.get_scheduler();
 
    // Helper: produce a sender that computes the partial sum of a slice
    auto make_partial = [&](std::size_t begin, std::size_t end) {
        return stdexec::schedule(sch)
             | stdexec::then([data, begin, end]() -> double {
                   double sum = 0.0;
                   for (std::size_t i = begin; i < end; ++i)
                       sum += data[i];
                   return sum;
               });
    };
 
    std::size_t chunk = data.size() / NTHREADS;
 
    auto t0 = std::chrono::steady_clock::now();
 
    // when_all accepts variadic senders; hard-code 8 for clarity
    auto joined = stdexec::when_all(
        make_partial(0 * chunk, 1 * chunk),
        make_partial(1 * chunk, 2 * chunk),
        make_partial(2 * chunk, 3 * chunk),
        make_partial(3 * chunk, 4 * chunk),
        make_partial(4 * chunk, 5 * chunk),
        make_partial(5 * chunk, 6 * chunk),
        make_partial(6 * chunk, 7 * chunk),
        make_partial(7 * chunk, data.size())  // last chunk may be larger
    );
 
    // sync_wait returns optional<tuple<double,double,...,double>> (8 doubles)
    auto result = stdexec::sync_wait(std::move(joined)); 
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
 
    // Sum the 8-element tuple via apply
    auto total = std::apply([](auto... vals) { return (vals + ... + 0.0); }, result.value());
 
    return std::make_tuple(total, elapsed);    
}
 
// ---------------------------------------------------------------------------
// Benchmark 3: transfer — hop schedulers mid-pipeline
//
//   Demonstrates scheduler agnosticism: compute on the thread pool, then
//   transfer the result to an inline scheduler for final aggregation.
//   In production you'd transfer to a GPU scheduler here.
// ---------------------------------------------------------------------------
std::tuple<double, double> bench_transfer(std::span<const double> data, exec::static_thread_pool& pool)
{
    auto sch        = pool.get_scheduler();
    auto inline_sch = stdexec::inline_scheduler{};      // runs on the caller thread
 
    auto t0 = std::chrono::steady_clock::now();
 
    std::vector<double> partials(NTHREADS, 0.0);
    std::size_t chunk = data.size() / NTHREADS;
 
    auto work =
        stdexec::schedule(sch)
      | stdexec::bulk(NTHREADS, [&](int t) {
            std::size_t begin = t * chunk;
            std::size_t end   = (t == NTHREADS - 1) ? data.size() : begin + chunk;
            double sum = 0.0;
            for (std::size_t i = begin; i < end; ++i)
                sum += data[i];
            partials[t] = sum;
        })
      | stdexec::transfer(inline_sch);   // ← hop: aggregation runs on caller thread
 
    stdexec::sync_wait(std::move(work));
 
    auto total = std::accumulate(std::cbegin(partials), std::cend(partials), 0.0f); 
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
 
    // One thread pool shared across all benchmarks — real cost paid once
    exec::static_thread_pool pool(NTHREADS);
 
    auto t1 = bench_bulk(view, pool);
    auto t2 = bench_when_all(view, pool);
    auto t3 = bench_transfer(view, pool);
 
    std::printf("\n--- Summary ---\n");
    std::printf("[C++26 / bulk     ] total: %.6f, time = %.4f ms\n", std::get<0>(t1), std::get<1>(t1));
    std::printf("[C++26 / when_all ] total: %.6f, time = %.4f ms\n", std::get<0>(t2), std::get<1>(t2));
    std::printf("[C++26 / transfer ] total: %.6f, time = %.4f ms\n", std::get<0>(t3), std::get<1>(t3));
 
    // Key lesson: all three achieve similar throughput because the scheduler
    // (static_thread_pool) is identical.  Swap pool for a GPU scheduler and
    // bench_bulk / bench_when_all accelerate with zero algorithm changes. 
    return 0;
}