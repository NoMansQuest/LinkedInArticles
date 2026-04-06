# High Performance Programming in C++: Article Demos

Hi, I'm Nasser, and welcome to the companion repository for my technical articles on LinkedIn. This repository is a collection of standalone "lab" projects designed to demonstrate deep-system concepts, ranging from cache coherency to modern C++ memory management.

These demos are built to show exactly how high-level C++ constructs interact with the underlying silicon. Hope you find them helpful.

---

## 📂 Repository Structure
Each folder is a self-contained project corresponding to a specific article. Each has its own `CMakeLists.txt` and is designed to be built independently.

* `01-False-Sharing/` – Demonstrates cache-line contention and the impact of `alignas`.
* `02-Polymorphic-Allocators/` – Comparing `std::pmr` (Polymorphic Memory Resources) against the global heap.

---

## 🛠 How to Build and Run
These demos are designed to be built individually. To ensure maximum performance, it is recommended to build in **Release** mode to allow the compiler to apply full hardware-specific optimizations.

### Prerequisites
* **Compiler:** A C++20 compliant compiler (GCC 11+, Clang 12+, or MSVC 19.29+).
* **Build System:** [CMake](https://cmake.org/download/) 3.15 or higher.

### Step-by-Step Instructions (in case you are new to CMake)

1. **Clone the repository:**
   ```bash
   git clone https://github.com/NoMansQuest/LinkedInArticles.git
   cd LinkedInArticles
   ```

2. **Navigate to the specific demo folder:**

    ```bash
    cd [FolderName]  # Example: cd 02-Polymorphic-Allocators
    ```

3. **Configure and Build:**

    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```

    This would create a 'build' folder and compile the app into an executable inside.

4. **Run the demo:**

    -   Linux/macOS: ./build/[ExecutableName]
    -   Windows: .\build\Release\[ExecutableName].exe

## 🔬 Hardware Awareness
Because these demos involve low-level performance metrics (cycles, cache hits, latency), results may vary based on your specific CPU architecture:

* **Cache Alignment:** Most modern CPUs use a 64-byte cache line. 
* **Thermal Throttling:** Ensure your system is not thermal throttling, as this will skew micro-benchmarks.
* **Instruction Sets:** The CMake configuration uses `-march=native` (on GCC/Clang) to leverage your specific CPU's capabilities.

## 🤝 Let's Connect
If you find these demos useful or have questions about the hardware-software trade-offs involved, let's discuss them on [LinkedIn!](https://www.linkedin.com/in/nasser-ghoseiri/).

**Nasser Ghoseiri** *Hardware-Software Co-Design | High-Performance C++ | FPGA RTL*

