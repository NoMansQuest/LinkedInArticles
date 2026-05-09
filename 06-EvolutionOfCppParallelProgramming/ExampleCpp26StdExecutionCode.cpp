#include <execution>
#include <exec/static_thread_pool.hpp>
#include <iostream>

// (Conceptual headers for a P2300-compatible networking library)
#include <net/io_context.hpp> 
#include <net/socket.hpp>

namespace ex = std::execution;

void process_network_request() {
    // 1. Two separate schedulers for two different jobs
    net::io_context io_ctx;                   // Highly tuned for epoll/io_uring
    exec::static_thread_pool cpu_pool(8);     // Highly tuned for CPU math/parsing    
    net::tcp_socket socket = /* ... connected socket ... */;
    std::vector<char> buffer(4096);

    // 2. Compose the pipeline: Network -> Transfer -> CPU Compute
    auto pipeline = 
        // AWAIT IO: Wait for bytes to arrive (Runs lazily on the io_ctx)
        net::async_read(socket, buffer) 
        
        // THE MAGIC HOP: Move execution from the I/O thread to the CPU pool
        | ex::transfer(cpu_pool.get_scheduler()) 
        
        // HEAVY LIFTING: Parse the data (Now safely running on the cpu_pool)
        | ex::then([&buffer](size_t bytes_read) {
            std::cout << "Parsing " << bytes_read << " bytes on thread: " 
                      << std::this_thread::get_id() << "\n";
            return parse_json_payload(buffer, bytes_read); 
        })
        
        // ERROR HANDLING: Catch socket disconnects or parsing errors cleanly
        | ex::upon_error([](std::exception_ptr e) {
            std::cerr << "Pipeline failed!\n";
            return empty_payload(); 
        });

    // 3. Start the engine
    io_ctx.run_in_background();
    
    // Connects the receiver and waits for the final payload
    auto [payload] = ex::sync_wait(std::move(pipeline)).value();
}