// =============================================================================
// bench_cpp98.cpp - Parallel reduction, C++98 style
//
// Compile (MSVC):  cl /O2 /std:c++14 bench_cpp98.cpp
//                  (MSVC has no /std:c++98 flag; c++14 is the earliest.
//                   The code itself is valid C++98 - no C++14 features used.)
//
// Compile (GCC/Clang, Linux/macOS):
//   g++ -O2 -std=c++98 -lpthread -o bench_cpp98 bench_cpp98.cpp
//   (change PLATFORM_WINDOWS to PLATFORM_POSIX below)
//
// =============================================================================
//
// C++98 had no standard threading library whatsoever. Developers used raw
// OS primitives directly - meaning code was not portable between platforms.
// This file shows both sides of that reality:
//
//   Windows : CreateThread / WaitForMultipleObjects (Win32 API)
//   POSIX   : pthread_create / pthread_join
//
// Pick the one matching your platform by setting the macro below.
// Everything else in the file is identical C++98.
//
// This non-portability is precisely what C++11 std::thread solved.
// =============================================================================

// -- Platform selection --------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#else
#define PLATFORM_POSIX
#endif

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

#include <cstddef>
#include <cstdio>

static const std::size_t N = 100000000;
static const int         NTHREADS = 8;

// Worker state - passed through the OS thread API as a void*
// (no lambdas in C++98; all context must be bundled into a struct)
struct ThreadArg {
    const double* data;
    std::size_t   begin;
    std::size_t   end;
    double        result;
};

// -- Worker function -----------------------------------------------------------
#ifdef PLATFORM_WINDOWS
static DWORD WINAPI worker(LPVOID raw)
{
    ThreadArg* arg = static_cast<ThreadArg*>(raw);
    double sum = 0.0;
    for (std::size_t i = arg->begin; i < arg->end; ++i)
        sum += arg->data[i];
    arg->result = sum;
    return 0;
}
#else
static void* worker(void* raw)
{
    ThreadArg* arg = static_cast<ThreadArg*>(raw);
    double sum = 0.0;
    for (std::size_t i = arg->begin; i < arg->end; ++i)
        sum += arg->data[i];
    arg->result = sum;
    return NULL;
}
#endif

// -- Portable wall-clock timer -------------------------------------------------
static double now_sec()
{
#ifdef PLATFORM_WINDOWS
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return static_cast<double>(count.QuadPart) / static_cast<double>(freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

// -- Benchmark -----------------------------------------------------------------
static double run(const double* data)
{
    std::size_t chunk = N / NTHREADS;
    ThreadArg   args[NTHREADS];

#ifdef PLATFORM_WINDOWS
    HANDLE handles[NTHREADS];

    double t0 = now_sec();

    for (int t = 0; t < NTHREADS; ++t) {
        args[t].data = data;
        args[t].begin = t * chunk;
        args[t].end = (t == NTHREADS - 1) ? N : (t + 1) * chunk;
        args[t].result = 0.0;
        handles[t] = CreateThread(
            NULL,       // default security
            0,          // default stack size
            worker,     // function pointer - no lambdas in C++98
            &args[t],   // void* argument
            0,          // run immediately
            NULL);      // don't need thread ID
    }

    WaitForMultipleObjects(NTHREADS, handles, TRUE, INFINITE);
    for (int t = 0; t < NTHREADS; ++t)
        CloseHandle(handles[t]);

#else
    pthread_t threads[NTHREADS];

    double t0 = now_sec();

    for (int t = 0; t < NTHREADS; ++t) {
        args[t].data = data;
        args[t].begin = t * chunk;
        args[t].end = (t == NTHREADS - 1) ? N : (t + 1) * chunk;
        args[t].result = 0.0;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }

    for (int t = 0; t < NTHREADS; ++t)
        pthread_join(threads[t], NULL);
#endif

    double total = 0.0;
    for (int t = 0; t < NTHREADS; ++t)
        total += args[t].result;

    double elapsed = now_sec() - t0;
    std::printf("[C++98 / OS threads]  total: %.4f,  time = %.4f ms   threads = %d\n",
        total, elapsed * 1000.0, NTHREADS);
    return elapsed;
}

int main()
{
    double* data = new double[N];
    for (std::size_t i = 0; i < N; ++i)
        data[i] = 1.0 / (i + 1.0);

    run(data);              // warm-up
    run(data);              // measured

    delete[] data;
    return 0;
}