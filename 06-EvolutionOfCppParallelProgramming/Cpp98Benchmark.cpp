// =============================================================================
// bench_cpp98.cpp  —  Parallel reduction, C++98 style (POSIX threads)
//
// Compile:  g++ -O2 -std=c++98 -lpthread -o bench_cpp98 bench_cpp98.cpp
// Run:      ./bench_cpp98
//
// Strategy: manually spawn N pthreads, each reduces its slice of the array,
//           then the main thread sums the per-thread partial results.
//           Every piece of RAII, synchronisation and work-splitting is hand-
//           written - exactly what developers had to do before C++11.
// ============================================================================= 
#include <pthread.h>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
 
// ---------------------------------------------------------------------------
// Benchmark parameters
// ---------------------------------------------------------------------------
static const std::size_t N = 5000000;  // 100 M elements
static const int NTHREADS = 8;         // tune to your core count
 
// ---------------------------------------------------------------------------
// Shared data passed to each worker thread via a plain struct
// (no lambdas, no std::function — C++98 only)
// ---------------------------------------------------------------------------
struct ThreadArg {
    const double* data;
    std::size_t   begin;
    std::size_t   end;
    double        result;   // written by the worker, read by main
};
 
static void* worker(void* raw)
{
    ThreadArg* arg = static_cast<ThreadArg*>(raw);
    double sum = 0.0;
    for (std::size_t i = arg->begin; i < arg->end; ++i)
        sum += arg->data[i];
    arg->result = sum;
    return NULL;
}
 
// ---------------------------------------------------------------------------
// Portable wall-clock timer (C89 clock() measures CPU time; we use
// clock_gettime where available, else fall back to clock()).
// ---------------------------------------------------------------------------
static double now_sec()
{
#ifdef _POSIX_TIMERS
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
#else
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
#endif
}
 
int main()
{
    // Allocate and initialise the array (raw new[], no vector in C++98 style
    // is fine, but std::vector existed — we use it via C-cast-free new here
    // to keep the "flavour" authentic).
    double* data = new double[N];
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);   // harmonic series - avoids trivial opt
 
    // -----------------------------------------------------------------------
    // Thread setup
    // -----------------------------------------------------------------------
    pthread_t threads[NTHREADS];
    ThreadArg args[NTHREADS]; 
    std::size_t chunk = N / NTHREADS;
 
    double t0 = now_sec();
 
    for (int t = 0; t < NTHREADS; ++t) {
        args[t].data   = data;
        args[t].begin  = t * chunk;
        args[t].end    = (t == NTHREADS - 1) ? N : (t + 1) * chunk;
        args[t].result = 0.0;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }
 
    // -----------------------------------------------------------------------
    // Join and combine
    // -----------------------------------------------------------------------
    double total = 0.0;
    for (int t = 0; t < NTHREADS; ++t) {
        pthread_join(threads[t], NULL);
        total += args[t].result;
    }
 
    double elapsed = (now_sec() - t0) * 1000; 
    std::printf("[C++98 / pthreads ] total = %.6f   time = %.4f ms   threads = %d\n", total, elapsed, NTHREADS);
 
    delete[] data;
    return 0;
}