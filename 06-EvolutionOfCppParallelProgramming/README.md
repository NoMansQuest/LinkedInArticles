# C++ Parallel Reduction Benchmark Suite
## C++98 -> C++11 -> C++17 -> C++26

### The Task

Each file performs a **parallel reduction** (sum) over **100 million `double` values** (the harmonic series `1/1 + 1/2 + 1/3 + … + 1/N`) using the idiomatic parallelism mechanism of its era. The workload is purely CPU-bound - no I/O, no allocation inside the hot loop - isolating threading overhead from every other variable.

---

### Why only four versions?

| Standard | Included | Reason                                                                         |
| -------- | -------- | ------------------------------------------------------------------------------ |
| C++98    | Y        | Baseline: full manual thread management                                        |
| C++11    | Y        | Ergonomics shift: `std::thread` + lambdas, same runtime cost                   |
| C++14    | N        | No new parallel primitives; ergonomics only (`make_unique`, generic lambdas)   |
| C++17    | Y        | First real performance jump: `par_unseq` adds threading + SIMD                 |
| C++20    | N        | No new parallel algorithms; `jthread`/`barrier`/`span` are safety improvements |
| C++23    | N        | Still `par_unseq`; ranges have no parallel overloads yet                       |
| C++26    | Y        | Architectural shift: scheduler becomes an explicit runtime parameter           |

C++14, C++20, and C++23 are genuinely non-events for parallel performance. Including them would imply a difference where none exists.

---

### Files

| File              | Standard | Mechanism                                              |
| ----------------- | -------- | ------------------------------------------------------ |
| `bench_cpp98.cpp` | C++98    | POSIX `pthread_create` / `pthread_join`                |
| `bench_cpp11.cpp` | C++11    | `std::thread` + lambdas                                |
| `bench_cpp17.cpp` | C++17    | `std::reduce(par_unseq)` - parallel STL                |
| `bench_cpp26.cpp` | C++26    | `std::reduce(par_unseq)` + `ex::bulk` (std::execution) |

---

### Compilation

```bash
# C++98
g++ -O2 -std=c++98 -lpthread -o bench_cpp98 bench_cpp98.cpp

# C++11
g++ -O2 -std=c++11 -o bench_cpp11 bench_cpp11.cpp

# C++17  (requires TBB)
g++ -O2 -std=c++17 -ltbb -o bench_cpp17 bench_cpp17.cpp

# C++26 - PATH A: Godbolt / clang trunk / gcc trunk (no external deps)
#   Compiler: x86-64 clang (trunk)
#   Flags:    -O2 -std=c++26 -pthread

# C++26 - PATH B: local build with stdexec
#   git clone https://github.com/NVIDIA/stdexec
g++ -O2 -std=c++26 -I<stdexec>/include -ltbb \
    -DUSE_STDEXEC -o bench_cpp26 bench_cpp26.cpp
```

**macOS:** replace `-ltbb` with `-L$(brew --prefix tbb)/lib -ltbb`  
**Windows / MSVC:** use `/std:c++17` or `/std:c++latest`; TBB from vcpkg

---

### Running

Every binary does one warm-up call (to pay any thread pool cold-start cost) followed by one measured call. Run each binary twice if you want to confirm the warm-up is working - the second run should be within noise of the first.

```bash
./bench_cpp98
./bench_cpp11
./bench_cpp17
./bench_cpp26
```

---

### The Story in Three Acts

#### Act 1 - C++98 -> C++11: portability, not speed

Both versions spawn OS threads manually, divide the array into chunks, and join after. The runtime cost is identical. What C++11 buys is portability (works on Windows without `#ifdef`), inline lambdas (no `void*` structs), and `<chrono>` (no `clock_gettime`). The threading infrastructure shrank from ~30 lines to ~10, but the OS is doing the same work.

**Expected result:** C++11 ≈ C++98

#### Act 2 - C++11 -> C++17: the only real performance jump

`std::reduce(par_unseq)` hands both thread management AND SIMD vectorisation to the runtime. The user writes one line; the STL backend (TBB) handles work-stealing, load balancing, NUMA placement, and auto-vectorisation. This is the inflection point of the entire progression.

**Expected result:** C++17 significantly faster than C++11 (2–5× depending on core count and SIMD width)

#### Act 3 - C++17 -> C++26: architecture, not speed

C++26 `std::execution` (P2300) does not make CPU reductions faster. The benchmark number for `par_unseq` is identical in C++17 and C++26. What changes is that the scheduler is now an explicit runtime parameter:

```cpp
// C++17 - scheduler implicit, fixed at compile time (TBB, hidden)
std::reduce(std::execution::par_unseq, begin, end, 0.0);

// C++26 - scheduler explicit, swappable at runtime
ex::schedule(sch) | ex::bulk(N, fn) | ex::sync_wait(…);
// swap 'sch' for a GPU scheduler -> runs on GPU, zero algorithm changes
```

**Expected result:** C++26 par_unseq ≈ C++17; C++26 bulk ≈ C++17

The flat line from C++17 to C++26 on your chart is not a failure -
it is the point. The improvement is in composability, safety, and
hardware portability, not raw throughput on a single CPU.

---

### Key Takeaway

> The only performance jump in 28 years of C++ parallelism happened in C++17. Everything before it was boilerplate. Everything after it is architecture.

---

### Benchmarking Tips

- Run on a **local machine**, not Godbolt - Godbolt is a shared virtualised
  environment that can't give representative parallel timings.
- Use `taskset -c 0-7 ./bench_cpp17` (Linux) to pin to physical cores.
- Run 3–5 times and take the median, not the minimum.
- `perf stat -e instructions,cycles ./bench_cpp17` shows IPC and confirms
  SIMD is active (high instructions-per-cycle in the par_unseq runs).
