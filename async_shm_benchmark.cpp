#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "yaml_parser.hpp"
#include "shared_memory_manager.hpp"
#include "collision_manager/collision_manager.hpp"
#include <benchmark/benchmark.h>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <atomic>
#include <vector>
#include <numeric>
#include <mutex>
#include <future>
#include <google/protobuf/empty.pb.h>

const std::string CSV_FILE = "../Motor_Vehicle_Collisions_-_Crashes_20250123.csv";

// Global variables for tracking scaling baselines
double async_shm_strong_scaling_baseline = 0.0;
double async_shm_weak_scaling_baseline = 0.0;
std::mutex benchmark_mutex;

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

// Utility to create a gRPC client stub for async server
std::unique_ptr<collision_proto::CollisionQueryService::Stub> createClientStub(const std::string& address) {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    return collision_proto::CollisionQueryService::NewStub(channel);
}

// Strong Scaling benchmark - connecting to running async shared memory servers
static void BM_AsyncShmServerStrongScaling(benchmark::State& state) {
    // Number of server instances to test with
    int num_servers = state.range(0);

    // Total workload remains fixed
    const int TOTAL_QUERIES = 10; // Reduced number for testing

    // Create test queries
    auto queries = createTestQueries(TOTAL_QUERIES);

    // Parse config
    DeploymentConfig config = parseConfig("../config.yaml");

    // Connect to the existing servers
    std::vector<std::unique_ptr<collision_proto::CollisionQueryService::Stub>> clients;
    for (int i = 0; i < num_servers; i++) {
        std::string server_address = "127.0.0.1:" + std::to_string(50051 + i);
        clients.push_back(createClientStub(server_address));
    }

    std::vector<double> server_times;

    for (auto _ : state) {
        state.PauseTiming();
        server_times.clear();
        state.ResumeTiming();

        // Run benchmark
        std::vector<std::thread> client_threads;
        for (int i = 0; i < num_servers; i++) {
            client_threads.emplace_back([&, server_id = i]() {
                auto start_time = std::chrono::high_resolution_clock::now();
                auto& client = clients[server_id];

                // Calculate the query partition for this client
                int partition_size = TOTAL_QUERIES / num_servers;
                int start_index = server_id * partition_size;
                int end_index = (server_id == num_servers - 1) ? TOTAL_QUERIES : (start_index + partition_size);

                for (int query_idx = start_index; query_idx < end_index; query_idx++) {
                    // For async server, we use SendRequest method
                    collision_proto::QueryRequest proto_request = QueryProtoConverter::serialize(queries[query_idx]);

                    // Execute query
                    grpc::ClientContext context;
                    google::protobuf::Empty response;
                    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
                    context.set_deadline(deadline);

                    auto status = client->SendRequest(&context, proto_request, &response);
                    benchmark::DoNotOptimize(response);

                    // Sleep a bit to prevent overwhelming the servers
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = end_time - start_time;

                std::lock_guard<std::mutex> lock(benchmark_mutex);
                server_times.push_back(duration.count());
            });
        }

        // Wait for all clients to finish
        for (auto& thread : client_threads) {
            thread.join();
        }
    }

    double avg_time = std::accumulate(server_times.begin(), server_times.end(), 0.0) / server_times.size();
    double max_time = *std::max_element(server_times.begin(), server_times.end());

    state.counters["Queries_per_second"] = benchmark::Counter(
        TOTAL_QUERIES, benchmark::Counter::kIsRate);
    state.counters["Avg_server_time"] = avg_time;
    state.counters["Max_server_time"] = max_time;

    if (num_servers > 1 && async_shm_strong_scaling_baseline > 0) {
        double speedup = async_shm_strong_scaling_baseline / max_time;
        double ideal_speedup = num_servers; // Linear scaling
        state.counters["Speedup"] = speedup;
        state.counters["Scaling_efficiency"] = benchmark::Counter(
            100.0 * (speedup / ideal_speedup)); // 100% = linear, >100% = super-linear
    } else if (num_servers == 1) {
        async_shm_strong_scaling_baseline = max_time;
        state.counters["Speedup"] = 1.0;
        state.counters["Scaling_efficiency"] = 100.0;
    }
}

// Weak Scaling benchmark - connecting to running async shared memory servers
static void BM_AsyncShmServerWeakScaling(benchmark::State& state) {
    int num_servers = state.range(0);
    const int QUERIES_PER_SERVER = 5; // Reduced for testing
    const int TOTAL_QUERIES = QUERIES_PER_SERVER * num_servers;

    // Create test queries
    auto queries = createTestQueries(TOTAL_QUERIES);

    // Connect to the existing servers
    std::vector<std::unique_ptr<collision_proto::CollisionQueryService::Stub>> clients;
    for (int i = 0; i < num_servers; i++) {
        std::string server_address = "127.0.0.1:" + std::to_string(50051 + i);
        clients.push_back(createClientStub(server_address));
    }

    std::vector<double> server_times;

    for (auto _ : state) {
        state.PauseTiming();
        server_times.clear();
        state.ResumeTiming();

        // Run benchmark
        std::vector<std::thread> client_threads;
        for (int i = 0; i < num_servers; i++) {
            client_threads.emplace_back([&, server_id = i]() {
                auto start_time = std::chrono::high_resolution_clock::now();
                auto& client = clients[server_id];

                int start_idx = server_id * QUERIES_PER_SERVER;
                int end_idx = start_idx + QUERIES_PER_SERVER;

                for (int query_idx = start_idx; query_idx < end_idx; query_idx++) {
                    // For async server, we use SendRequest method
                    collision_proto::QueryRequest proto_request = QueryProtoConverter::serialize(queries[query_idx]);

                    // Execute query
                    grpc::ClientContext context;
                    google::protobuf::Empty response;
                    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
                    context.set_deadline(deadline);

                    auto status = client->SendRequest(&context, proto_request, &response);
                    benchmark::DoNotOptimize(response);

                    // Sleep a bit to prevent overwhelming the servers
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = end_time - start_time;

                std::lock_guard<std::mutex> lock(benchmark_mutex);
                server_times.push_back(duration.count());
            });
        }

        // Wait for all clients to finish
        for (auto& thread : client_threads) {
            thread.join();
        }
    }

    double avg_time = std::accumulate(server_times.begin(), server_times.end(), 0.0) / server_times.size();
    double avg_time_per_query = avg_time / QUERIES_PER_SERVER;

    state.counters["Total_queries"] = TOTAL_QUERIES;
    state.counters["Queries_per_server"] = QUERIES_PER_SERVER;
    state.counters["Avg_time_per_query_ms"] = avg_time_per_query * 1000.0;

    if (num_servers > 1 && async_shm_weak_scaling_baseline > 0) {
        double efficiency = async_shm_weak_scaling_baseline / avg_time_per_query;
        state.counters["Weak_scaling_efficiency"] = benchmark::Counter(
            100.0 * efficiency); // 100% = perfect, <100% = efficiency loss
    } else if (num_servers == 1) {
        async_shm_weak_scaling_baseline = avg_time_per_query;
        state.counters["Weak_scaling_efficiency"] = 100.0;
    }
}

// Run benchmarks
BENCHMARK(BM_AsyncShmServerStrongScaling)
    ->Range(1, 5)  // Test with 1 to 5 servers
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(BM_AsyncShmServerWeakScaling)
    ->Range(1, 5)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();