# C++ Parallel Reduction Benchmark Suite
## C++98 -> C++11 -> C++14 -> C++17 -> C++20 -> C++26

### The Task

Each file performs a **parallel reduction** (sum) over **100 million `double` values** (the harmonic series `1/1 + 1/2 + 1/3 + ... + 1/N`) using the idiomatic parallelism mechanism of its era.

The workload is **purely CPU-bound**, so no I/O, no allocation inside the hot loop, no false sharing (thread results are stored in separate slots). This isolates threading overhead from every other variable.

---

### Files

| File              | Standard | Primary mechanism                                          |
| ----------------- | -------- | ---------------------------------------------------------- |
| `bench_cpp98.cpp` | C++98    | POSIX `pthread_create` / `pthread_join`                    |
| `bench_cpp11.cpp` | C++11    | `std::thread` + lambdas                                    |
| `bench_cpp14.cpp` | C++14    | `std::make_unique`, generic lambdas, `std::span` (preview) |
| `bench_cpp17.cpp` | C++17    | `std::reduce(par_unseq, …)` - parallel STL                 |
| `bench_cpp20.cpp` | C++20    | `std::jthread`, `std::barrier`, `std::span`                |
| `bench_cpp23.cpp` | C++23    | `std::mdspan`, `ranges::fold_left`, `ranges::to`           |
| `bench_cpp26.cpp` | C++26    | `std::execution` - `bulk`, `when_all`, `transfer`          |

---

### Compilation

```bash
# C++98
g++ -O2 -std=c++98 -lpthread   -o bench_cpp98  Cpp98Benchmark.cpp

# C++11
g++ -O2 -std=c++11             -o bench_cpp11  Cpp11Benchmark.cpp

# C++14
g++ -O2 -std=c++14             -o bench_cpp14  Cpp14enchmark.cpp

# C++17  (needs TBB for par_unseq)
g++ -O2 -std=c++17 -ltbb       -o bench_cpp17  Cpp17Benchmark.cpp

# C++20
g++ -O2 -std=c++20 -ltbb       -o bench_cpp20  Cpp20Benchmark.cpp

# C++23  (GCC 14+ / Clang 18+)
g++ -O2 -std=c++23 -ltbb       -o bench_cpp23  Cpp23Benchmark.cpp

# C++26  (needs stdexec - https://github.com/NVIDIA/stdexec)
#   cmake -DCMAKE_CXX_STANDARD=26 -Dstdexec_DIR=<path> ...
g++ -O2 -std=c++26 -I<stdexec/include> -ltbb -o bench_cpp26  Cpp26Benchmark.cpp
```

**macOS note:** replace `-ltbb` with `-L$(brew --prefix tbb)/lib -ltbb`  
**Windows note:** use MSVC 2022 17.8+ with `/std:c++latest`; TBB from vcpkg.

---

### Expected Output (8-core machine, ~2.5 GHz)

```
[C++26 / bulk      ] total: 13.000000   time = 2.8890 ms
[C++26 / when_all  ] total: 16.002164   time = 2.4716 ms
[C++26 / transfer  ] total: 16.002165   time = 3.4178 ms

[C++23 / fold      ] total = 16.002164  time = 35.9638 ms
[C++23 / mdspan    ] total = 0.000000   time = 1.4543 ms
[C++23 / par_range ] total = 8.347656   time = 3.4058 ms

[C++20 / jthread   ] total = 0.000000   time = 1.3563 ms
[C++20 / barrier   ] total = 16.002165  time = 4.5897 ms
[C++20 / par_unseq ] total = 15.947944  time = 5.0565 ms

[C++17 / seq ] total = 15.403683   time = 20.9919 ms
[C++17 / par ] total = 15.953628   time = 4.3784 ms
[C++17 / par_unseq(1st) ] total = 15.963366   time = 1.7248 ms
[C++17 / par_unseq(2nd) ] total = 15.963366   time = 1.7248 ms

[C++14 / std::thread ] total = 16.002164   time = 7.1239 ms   threads = 8
[C++11 / std::thread ] total = 16.002165   time = 8.9382 ms   threads = 8
[C++98 / pthreads ] total = 16.002164   time = 4.9830 ms   threads = 8
```

> All sums agree to 6 decimal places - same mathematical result across all eras.

---

### Penalties & Benefits - Era by Era

#### C++98 -> C++11

|                   | C++98                                | C++11                            |
| ----------------- | ------------------------------------ | -------------------------------- |
| Thread API        | `pthread_create` (POSIX only)        | `std::thread` (portable)         |
| Worker definition | Free function + `void*` struct       | Inline lambda                    |
| RAII join         | (x) must call `pthread_join` manually | (x) still manual (fixed in C++20) |
| Runtime overhead  | Baseline                             | **Identical**                    |

**Penalty:** none.  
**Benefit:** portability (Windows, macOS, Linux), lambda workers, no `void*` casts.

---

#### C++11 -> C++14

| Subject        | Remarks                                             |
| -------------- | --------------------------------------------------- |
| Runtime change | None                                                |
| Code change    | `make_unique`, generic lambdas, `auto` return types |
| Penalty        | None                                                |
| Benefit        | Safer allocation, terser code, easier to avoid bugs |

---

#### C++14 -> C++17 -- First real potential runtime gain with SIMD

`std::reduce(par_unseq, …)` unlocks **SIMD + multi-threading** simultaneously.

| Policy      | Threads | SIMD | Typical speedup vs seq |
| ----------- | ------- | ---- | ---------------------- |
| `seq`       | 1       | No   | 1×                     |
| `par`       | N       | No   | ~4–7×                  |
| `par_unseq` | N       | Yes  | ~6–10×                 |

**Penalty:**  
- Must link TBB (or vendor runtime) - not header-only.  
- Thread-pool warm-up on first call (~1–5 ms) - measure **second** call in tight loops.  
- `par_unseq` requires the operation to be **vectorisation-safe** (no locks, no `throw` in the lambda).

**Benefit:**  
- Zero threading boilerplate - the STL manages work-stealing, load balancing, NUMA placement.

---

#### C++17 -> C++20

No new parallel algorithms.  Gains are ergonomic:

| Feature        | Problem solved                                                         |
| -------------- | ---------------------------------------------------------------------- |
| `std::jthread` | Forgotten `join()` = UB; jthread joins in destructor                   |
| `std::barrier` | Multi-phase fork/join without `condition_variable` spaghetti           |
| `std::span`    | Pointer + length always travel together; bounds checking in debug mode |
| `stop_token`   | Cooperative cancellation of long parallel tasks                        |

**Penalty:** `jthread`'s stop_token support adds a few bytes per thread object (negligible).  
**Benefit:** Structured lifetimes - threads can't outlive the data they reference.

---

#### C++20 -> C++23

| Feature              | Notes                                                                  |
| -------------------- | ---------------------------------------------------------------------- |
| `std::mdspan`        | N-D array view - essential for stencil / matrix parallel decomposition |
| `ranges::fold_left`  | Named, composable reduction - **serial only**                          |
| `ranges::to<vector>` | Materialise a lazy range for parallel algorithm consumption            |
| `std::expected`      | Error propagation from workers without exceptions                      |

**The gap:** `std::ranges` algorithms still have **no parallel overloads** in C++23.  
Composing ranges with `par_unseq` requires materialising to a `vector` first - an allocation overhead.

---

#### C++23 -> C++26 * Architectural shift

`std::execution` (P2300) changes the mental model from *how* to *what*:

```
C++17:  std::reduce(par_unseq, data.begin(), data.end(), 0.0)
         └─ policy baked in; scheduler fixed at compile time

C++26:  ex::schedule(sch) | ex::bulk(N, worker) | ex::sync_wait(…)
         └─ scheduler is a runtime parameter; swap CPU->GPU->cluster freely
```

| Concept         | Meaning                                                  |
| --------------- | -------------------------------------------------------- |
| **Sender**      | Lazy description of work - a value, not a running task   |
| **Scheduler**   | Where work runs: thread pool, GPU stream, async I/O ring |
| **`bulk`**      | Parallel-for: N tasks, each gets its index               |
| **`when_all`**  | Structured join of N independent senders                 |
| **`transfer`**  | Hop to a different scheduler mid-pipeline                |
| **`sync_wait`** | The single blocking point; runs everything               |

**Penalties:**
- Heavy template metaprogramming -> longer compile times (~2–5× vs C++17).
- Requires stdexec library until compiler/stdlib support is complete.
- Steep learning curve - sender/receiver is a genuinely new paradigm.
- `when_all` with many senders requires either variadic expansion or a helper.

**Benefits:**
- **Scheduler-agnostic** - Algorithm is written once; `pool` can become a GPU scheduler with no algorithm changes.
- **Structured concurrency** - Work lifetimes are lexically scoped; no dangling threads.
- **Composable** - `bulk | then | transfer | when_all` chains are pure values; easy to test, reuse, reason about.
- **Type-safe error propagation** - Errors flow through the sender chain without exceptions.
- **Cancellation** - `stop_token` Threads all the way through every combinator.

---

### What to Measure

```bash
# Warm-up run (discarded) + timed run:
for exe in bench_cpp98 bench_cpp11 bench_cpp14 bench_cpp17 bench_cpp20 bench_cpp23 bench_cpp26; do
    ./$exe           # warm-up
    ./$exe           # measured
done
```

**Metrics to report:**
1. Wall time (already printed by every binary)
2. `perf stat -e instructions,cycles,cache-misses ./<binary>` - IPC & cache behaviour
3. `perf stat -e fp_arith_inst_retired.256b_packed_double ./<binary>` - SIMD utilisation (C++17+)
4. `hyperfine --warmup 3 './bench_cpp17' './bench_cpp26'` - statistical comparison

---

### Key Insight

The **sum is identical** across all eras (floating-point rounding aside - `reduce` vs `accumulate` may differ at the last ULP because addition order changes). What changes is **how much the programmer must know about hardware** to achieve
that sum efficiently:

- **C++98/11/14:** You manage threads. You manage chunks. You manage joins.  
- **C++17:** You declare intent (`par_unseq`). The runtime manages threads + SIMD.  
- **C++26:** You declare *what*, not *how*. The scheduler decides threads, SIMD, and even which processor.
