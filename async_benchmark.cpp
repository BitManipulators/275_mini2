#include "collision_query_service_impl.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "collision_manager/collision_manager.hpp"
#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>
#include <vector>
#include <numeric>
#include <mutex>

const std::string CSV_FILE = "../Motor_Vehicle_Collisions_-_Crashes_20250123.csv";

// Global variables for tracking scaling baselines
double strong_scaling_baseline = 0.0;
double weak_scaling_baseline = 0.0;
std::mutex worker_mutex;

// Helper to create test queries
std::vector<QueryRequest> createTestQueries(int count) {
    std::vector<std::string> search_terms = {
        "Manhattan", "Brooklyn", "Queens", "Bronx", "Staten Island"
    };
    std::vector<QueryRequest> queries;
    for (int i = 0; i < count; i++) {
        QueryRequest query_request;
        query_request.id = i + 1000;

        // Create a Query for the BOROUGH field
        Query query = Query::create(
            CollisionField::BOROUGH,
            QueryType::CONTAINS,
            std::string(search_terms[i % search_terms.size()])
        );

        query_request.query = query;
        query_request.requested_by.push_back(0);
        queries.push_back(query_request);
    }
    return queries;
}
// Simulated process network based on config.yaml overlay
struct ProcessNetwork {
    // Store connections based on the overlay from config.yaml
    std::vector<std::vector<int>> connections = {
        {1},        // Process A (process_a) connects to B (process_b)
        {2, 3},     // Process B (process_b) connects to C (process_c) and D (process_d)
        {4},        // Process C (process_c) connects to E (process_e)
        {4},        // Process D (process_d) connects to E (process_e)
        {}          // Process E (process_e) has no outgoing connections
    };

    // Process ports from config.yaml
    std::vector<int> ports = {
        50051,      // Process A (process_a)
        50052,      // Process B (process_b)
        50053,      // Process C (process_c)
        50054,      // Process D (process_d)
        50055       // Process E (process_e)
    };

    // Simulates sending a message from one process to another
    void sendMessage(int from, int to, const QueryRequest& query) {
        // a placeholder for the network structure
    }

    // Get port number for a process
    int getPort(int process_id) {
        if (process_id >= 0 && process_id < ports.size()) {
            return ports[process_id];
        }
        return -1;
    }
};

// Strong Scaling: Fixed total workload, variable number of processes
static void BM_StrongScaling(benchmark::State& state) {
    // Number of processes to simulate
    int num_processes = state.range(0);

    // Total workload remains fixed
    const int TOTAL_QUERIES = 1000;

    auto collision_manager = std::make_unique<CollisionManager>(CSV_FILE);
    std::vector<std::unique_ptr<CollisionManager>> managers;
    for (int i = 0; i < num_processes; i++) {
        managers.push_back(std::make_unique<CollisionManager>(CSV_FILE));
    }
    auto queries = createTestQueries(TOTAL_QUERIES);
    ProcessNetwork network;

    std::atomic<int> processed_queries(0);
    std::vector<double> process_times;

    for (auto _ : state) {
        state.PauseTiming();
        processed_queries = 0;
        process_times.clear();
        std::vector<std::thread> processes;
        state.ResumeTiming();

       // Launch simulated processes
        for (int i = 0; i < num_processes; i++)
        {
            processes.emplace_back([&, process_id = i]()
                                   {
        auto start_time = std::chrono::high_resolution_clock::now();
        // Each thread uses its own manager instance
        auto& collision_manager = managers[process_id];
        // Calculate the query partition for this rank
        int partitionSize = TOTAL_QUERIES / num_processes;
        int start_index = process_id * partitionSize;
        int end_index = (process_id == num_processes - 1) ? TOTAL_QUERIES : (start_index + partitionSize);

        for (int query_idx = start_index; query_idx < end_index; query_idx++) {
            auto results = collision_manager->search(queries[query_idx].query);
            benchmark::DoNotOptimize(results);

            for (int connected : network.connections[process_id]) {
                if (connected < num_processes) {
                    network.sendMessage(process_id, connected, queries[query_idx]);
                }
            }
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;
        std::lock_guard<std::mutex> lock(worker_mutex);
        process_times.push_back(duration.count()); });
        }

        for (auto& process : processes) {
            process.join();
        }
    }

    double avg_time = std::accumulate(process_times.begin(), process_times.end(), 0.0) / process_times.size();
    double max_time = *std::max_element(process_times.begin(), process_times.end());

    state.counters["Queries_per_second"] = benchmark::Counter(
        TOTAL_QUERIES, benchmark::Counter::kIsRate);
    state.counters["Avg_process_time"] = avg_time;
    state.counters["Max_process_time"] = max_time;

    if (num_processes > 1 && strong_scaling_baseline > 0) {
        double speedup = strong_scaling_baseline / max_time;
        double ideal_speedup = num_processes; // Linear scaling
        state.counters["Speedup"] = speedup;
        state.counters["Scaling_efficiency"] = benchmark::Counter(
            100.0 * (speedup / ideal_speedup)); // 100% = linear, >100% = super-linear
    } else if (num_processes == 1) {
        strong_scaling_baseline = max_time;
        state.counters["Speedup"] = 1.0;
        state.counters["Scaling_efficiency"] = 100.0;
    }
}

// Weak Scaling: Workload increases with number of processes
static void BM_WeakScaling(benchmark::State& state) {
    int num_processes = state.range(0);
    const int QUERIES_PER_PROCESS = 2000;
    const int TOTAL_QUERIES = QUERIES_PER_PROCESS * num_processes;

    auto collision_manager = std::make_unique<CollisionManager>(CSV_FILE);
    auto queries = createTestQueries(TOTAL_QUERIES);
    ProcessNetwork network;

    std::atomic<int> processed_queries(0);
    std::vector<double> process_times;

    for (auto _ : state) {
        state.PauseTiming();
        processed_queries = 0;
        process_times.clear();
        std::vector<std::thread> processes;
        state.ResumeTiming();

        // Launch simulated processes
        for (int i = 0; i < num_processes; i++) {
            processes.emplace_back([&, process_id = i]() {
                auto start_time = std::chrono::high_resolution_clock::now();

                int start_idx = process_id * QUERIES_PER_PROCESS;
                int end_idx = start_idx + QUERIES_PER_PROCESS;

                for (int query_idx = start_idx; query_idx < end_idx; query_idx++) {
                    auto results = collision_manager->search(queries[query_idx].query);
                    benchmark::DoNotOptimize(results);
                    processed_queries++;

                    // Simulate forwarding query through network
                    for (int connected : network.connections[process_id % 5]) {
                        if (connected < num_processes) {
                            network.sendMessage(process_id, connected, queries[query_idx]);
                        }
                    }
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = end_time - start_time;

                std::lock_guard<std::mutex> lock(worker_mutex);
                process_times.push_back(duration.count());
            });
        }

        for (auto& process : processes) {
            process.join();
        }
    }

    double avg_time = std::accumulate(process_times.begin(), process_times.end(), 0.0) / process_times.size();
    double avg_time_per_query = avg_time / QUERIES_PER_PROCESS;

    state.counters["Total_queries"] = TOTAL_QUERIES;
    state.counters["Queries_per_process"] = QUERIES_PER_PROCESS;
    state.counters["Avg_time_per_query_ms"] = avg_time_per_query * 1000.0;

    if (num_processes > 1 && weak_scaling_baseline > 0) {
        double efficiency = weak_scaling_baseline / avg_time_per_query;
        state.counters["Weak_scaling_efficiency"] = benchmark::Counter(
            100.0 * efficiency); // 100% = perfect, <100% = efficiency loss
    } else if (num_processes == 1) {
        weak_scaling_baseline = avg_time_per_query;
        state.counters["Weak_scaling_efficiency"] = 100.0;
    }
}

// Benchmark using actual process configuration from config.yaml
static void BM_ConfigTopology(benchmark::State& state) {
    const int NUM_PROCESSES = 5;
    const int QUERIES_PER_PROCESS = state.range(0);
    const int TOTAL_QUERIES = QUERIES_PER_PROCESS * NUM_PROCESSES;

    auto collision_manager = std::make_unique<CollisionManager>(CSV_FILE);
    auto queries = createTestQueries(TOTAL_QUERIES);
    ProcessNetwork network;

    // Track metrics for communication and query processing
    std::atomic<int> total_messages(0);
    std::atomic<int> processed_queries(0);

    for (auto _ : state) {
        state.PauseTiming();
        total_messages = 0;
        processed_queries = 0;
        std::vector<std::thread> processes;
        state.ResumeTiming();

        // Simulated distributed execution across 5 processes
        // Each process runs in its own thread
        for (int i = 0; i < NUM_PROCESSES; i++) {
            processes.emplace_back([&, process_id = i]() {
                // Each process handles its share of queries
                int start_idx = process_id * QUERIES_PER_PROCESS;
                int end_idx = start_idx + QUERIES_PER_PROCESS;

                for (int query_idx = start_idx; query_idx < end_idx; query_idx++) {
                    auto results = collision_manager->search(queries[query_idx].query);
                    benchmark::DoNotOptimize(results);
                    processed_queries++;

                    // Simulate network communication based on overlay topology
                    for (int connected : network.connections[process_id]) {
                        network.sendMessage(process_id, connected, queries[query_idx]);
                        total_messages++;
                    }
                }
            });
        }

        for (auto& process : processes) {
            process.join();
        }
    }

    state.counters["Total_queries"] = TOTAL_QUERIES;
    state.counters["Total_messages"] = total_messages.load();
    state.counters["Messages_per_query"] = static_cast<double>(total_messages) / TOTAL_QUERIES;
}

// Run benchmarks
BENCHMARK(BM_StrongScaling)
    ->Range(1, 5)  // Test with 1 to 5 processes
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(BM_WeakScaling)
    ->Range(1, 5)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(BM_ConfigTopology)
    ->RangeMultiplier(2)
    ->Range(50, 400)  // Queries per process
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();