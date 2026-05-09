// =============================================================================
// bench_cpp98.cpp - Parallel reduction, C++98 style (POSIX threads)
//
// Compile:  g++ -O2 -std=c++98 -lpthread -o bench_cpp98 bench_cpp98.cpp
// Run:      ./bench_cpp98
//
// Mechanism: manually spawn NTHREADS pthreads, each reduces its slice,
//            main thread joins all and sums the partial results.
//
// This is the complete parallel programming model in C++98:
//   - No standard thread type; must use OS primitives directly (POSIX here,
//     Win32 CreateThread on Windows - no portability)
//   - Worker logic passed as a function pointer through a void*
//   - Worker state bundled into a hand-rolled struct; no lambdas
//   - No RAII; forgetting pthread_join leaks the thread
//   - No standard timer; clock_gettime is POSIX-only
//
// Every line below is load-bearing infrastructure. None of it is the
// algorithm - the algorithm is four lines inside worker().
// =============================================================================
 
#include <pthread.h>
#include <cstddef>
#include <cstdio>
#include <ctime>
 
static const std::size_t N = 10000000;
static const int NTHREADS = 8;
 
struct ThreadArg {
    const double* data;
    std::size_t   begin;
    std::size_t   end;
    double        result;
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
 
static double now_sec()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
 
static double run(const double* data)
{
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
 
    double total = 0.0;
    for (int t = 0; t < NTHREADS; ++t) {
        pthread_join(threads[t], NULL);
        total += args[t].result;
    }
 
    return (now_sec() - t0) * 1000; // Obtain time in milliseconds
}
 
int main()
{
    double* data = new double[N];
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);
 
    run(data);                          // warm-up
    double elapsed = run(data);         // measured
 
    std::printf("[C++98 / pthreads]  time = %.4f ms   threads = %d\n",
                elapsed, NTHREADS);
 
    delete[] data;
    return 0;
}