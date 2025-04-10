#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

// Base port for each server type
const int SYNC_BASE_PORT = 50051;
// const int ASYNC_BASE_PORT = 60051;
// const int ASYNC_SHM_BASE_PORT = 70051;

class DistributedServerTest : public ::testing::Test {
protected:
    // Timeout for gRPC calls (in seconds)
    const int RPC_TIMEOUT = 10;

    // Helper function to create a test query that should return data from all nodes
    Query createTestQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::CONTAINS, "");
    }

    // Helper function to create a query specific to Brooklyn with ZIP 11233
    Query createBrooklynQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
            .add(CollisionField::ZIP_CODE, QueryType::EQUALS, static_cast<uint32_t>(11233));
    }

    // Helper function to create a query for Manhattan
    Query createManhattanQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "MANHATTAN");
    }

    // Helper function to query a server
    std::vector<Collision> queryServer(int base_port, const Query& query) {
        // We always query process A (rank 0), which is at base_port
        std::string target_str = "127.0.0.1:" + std::to_string(base_port);
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
            target_str, grpc::InsecureChannelCredentials());
        std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub =
            collision_proto::CollisionQueryService::NewStub(channel);

        QueryRequest query_request = {
            .id = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()),
            .requested_by = {std::numeric_limits<std::uint32_t>::max()}, // Client ID
            .query = query,
        };

        collision_proto::QueryRequest request = QueryProtoConverter::serialize(query_request);

        collision_proto::QueryResponse response;
        grpc::ClientContext context;

        // Set timeout
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(RPC_TIMEOUT);
        context.set_deadline(deadline);

        std::cout << "Sending query to server at port " << base_port << std::endl;
        grpc::Status status = stub->GetCollisions(&context, request, &response);

        if (status.ok()) {
            auto query_response = CollisionProtoConverter::deserialize(response);
            std::cout << "Received " << query_response.collisions.size()
                      << " collisions from server at port " << base_port << std::endl;
            return query_response.collisions;
        } else {
            std::cerr << "RPC Error on port " << base_port << ": "
                      << status.error_code() << ": " << status.error_message()
                      << " (" << status.error_details() << ")" << std::endl;
            return {};
        }
    }

    // Helper function to compare collision records
    bool compareCollisions(const std::vector<Collision>& a, const std::vector<Collision>& b) {
        if (a.size() != b.size()) {
            std::cout << "Size mismatch: " << a.size() << " vs " << b.size() << std::endl;
            return false;
        }

        // Create sets of collision IDs for easy comparison
        std::unordered_set<std::size_t> a_ids, b_ids;

        for (const auto& collision : a) {
            if (collision.collision_id.has_value()) {
                a_ids.insert(collision.collision_id.value());
            }
        }

        for (const auto& collision : b) {
            if (collision.collision_id.has_value()) {
                b_ids.insert(collision.collision_id.value());
            }
        }

        // Count the differences
        int missing_in_b = 0;
        for (const auto& id : a_ids) {
            if (b_ids.find(id) == b_ids.end()) {
                missing_in_b++;
                if (missing_in_b <= 5) { // Only print first 5 differences
                    std::cout << "Collision ID " << id << " found in first set but not in second" << std::endl;
                }
            }
        }

        int missing_in_a = 0;
        for (const auto& id : b_ids) {
            if (a_ids.find(id) == a_ids.end()) {
                missing_in_a++;
                if (missing_in_a <= 5) { // Only print first 5 differences
                    std::cout << "Collision ID " << id << " found in second set but not in first" << std::endl;
                }
            }
        }

        if (missing_in_a > 0 || missing_in_b > 0) {
            std::cout << "Total differences: " << missing_in_a + missing_in_b
                      << " (Missing in first: " << missing_in_a
                      << ", Missing in second: " << missing_in_b << ")" << std::endl;
            return false;
        }

        return true;
    }

    // Helper to log statistics about the results
    void logResults(const std::string& server_type, const std::vector<Collision>& collisions) {
        std::cout << "==== " << server_type << " Results ====" << std::endl;
        std::cout << "Total collisions: " << collisions.size() << std::endl;

        // Count by borough
        std::unordered_map<std::string, int> borough_counts;
        std::unordered_map<uint32_t, int> zip_counts;

        for (const auto& collision : collisions) {
            if (collision.borough.has_value()) {
                // Use the FixedString directly as a C-string
                const auto& borough_val = collision.borough.value();
                // Convert to std::string for use as map key
                std::string borough_str(borough_val.c_str());
                borough_counts[borough_str]++;
            } else {
                borough_counts["UNKNOWN"]++;
            }

            if (collision.zip_code.has_value()) {
                zip_counts[collision.zip_code.value()]++;
            }
        }

        std::cout << "Borough distribution:" << std::endl;
        for (const auto& [borough, count] : borough_counts) {
            std::cout << "  " << borough << ": " << count << std::endl;
        }

        // Print top 5 zip codes
        std::vector<std::pair<uint32_t, int>> zip_vec(zip_counts.begin(), zip_counts.end());
        std::sort(zip_vec.begin(), zip_vec.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        std::cout << "Top 5 ZIP codes:" << std::endl;
        for (size_t i = 0; i < std::min(5UL, zip_vec.size()); i++) {
            std::cout << "  " << zip_vec[i].first << ": " << zip_vec[i].second << std::endl;
        }

        // Print first few records for manual verification
        const int MAX_RECORDS_TO_PRINT = 5;
        std::cout << "First " << std::min(MAX_RECORDS_TO_PRINT, static_cast<int>(collisions.size()))
                 << " records:" << std::endl;

        for (size_t i = 0; i < std::min(static_cast<size_t>(MAX_RECORDS_TO_PRINT), collisions.size()); i++) {
            const auto& collision = collisions[i];
            std::cout << "  " << i + 1 << ". ID: ";

            if (collision.collision_id.has_value()) {
                std::cout << collision.collision_id.value();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << ", Borough: ";
            if (collision.borough.has_value()) {
                // Use c_str() to access the character data
                std::cout << collision.borough.value().c_str();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << ", ZIP: ";
            if (collision.zip_code.has_value()) {
                std::cout << collision.zip_code.value();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << std::endl;
        }

        std::cout << std::endl;
    }

    // Verify that a set of collisions match a specific borough and ZIP code
    bool verifyBrooklynCollisions(const std::vector<Collision>& collisions) {
        for (const auto& collision : collisions) {
            bool is_brooklyn = collision.borough.has_value() &&
                               std::string(collision.borough.value().c_str()) == "BROOKLYN";

            bool has_zip = collision.zip_code.has_value() &&
                          collision.zip_code.value() == 11233;

            if (!is_brooklyn || !has_zip) {
                std::cout << "Invalid record found - Borough: ";
                if (collision.borough.has_value()) {
                    std::cout << collision.borough.value().c_str();
                } else {
                    std::cout << "UNKNOWN";
                }

                std::cout << ", ZIP: ";
                if (collision.zip_code.has_value()) {
                    std::cout << collision.zip_code.value();
                } else {
                    std::cout << "UNKNOWN";
                }

                std::cout << std::endl;
                return false;
            }
        }
        return true;
    }

    // Verify that a set of collisions match Manhattan
    bool verifyManhattanCollisions(const std::vector<Collision>& collisions) {
        for (const auto& collision : collisions) {
            bool is_manhattan = collision.borough.has_value() &&
                               std::string(collision.borough.value().c_str()) == "MANHATTAN";

            if (!is_manhattan) {
                std::cout << "Invalid record found - Borough: ";
                if (collision.borough.has_value()) {
                    std::cout << collision.borough.value().c_str();
                } else {
                    std::cout << "UNKNOWN";
                }
                std::cout << std::endl;
                return false;
            }
        }
        return true;
    }
};

// Test that all servers return consistent results for a query
TEST_F(DistributedServerTest, ServersReturnConsistentResults) {
    // We'll test with the Brooklyn+ZIP query which should give manageable results
    Query query = createBrooklynQuery();

    // Query each server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> sync_results = queryServer(SYNC_BASE_PORT, query);
    auto sync_end_time = std::chrono::high_resolution_clock::now();

    // Wait a bit between tests to let the system settle
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // auto async_start_time = std::chrono::high_resolution_clock::now();
    // std::vector<Collision> async_results = queryServer(ASYNC_BASE_PORT, query);
    // auto async_end_time = std::chrono::high_resolution_clock::now();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // auto shm_start_time = std::chrono::high_resolution_clock::now();
    // std::vector<Collision> async_shm_results = queryServer(ASYNC_SHM_BASE_PORT, query);
    // auto shm_end_time = std::chrono::high_resolution_clock::now();

    // Make sure we got some results from each
    ASSERT_FALSE(sync_results.empty()) << "Sync server returned no results";
    // ASSERT_FALSE(async_results.empty()) << "Async server returned no results";
    // ASSERT_FALSE(async_shm_results.empty()) << "Async SHM server returned no results";

    // Verify that results are for Brooklyn with ZIP 11233
    EXPECT_TRUE(verifyBrooklynCollisions(sync_results))
        << "Sync server returned records that don't match query criteria";
    // EXPECT_TRUE(verifyBrooklynCollisions(async_results))
    //     << "Async server returned records that don't match query criteria";
    // EXPECT_TRUE(verifyBrooklynCollisions(async_shm_results))
    //     << "Async SHM server returned records that don't match query criteria";

    // Log detailed results
    logResults("Sync Server", sync_results);
    // logResults("Async Server", async_results);
    // logResults("Async SHM Server", async_shm_results);

    // Compare the results - all should return the same records
    // EXPECT_TRUE(compareCollisions(sync_results, async_results))
    //     << "Sync and async server results don't match";

    // EXPECT_TRUE(compareCollisions(sync_results, async_shm_results))
    //     << "Sync and async SHM server results don't match";

    // EXPECT_TRUE(compareCollisions(async_results, async_shm_results))
    //     << "Async and async SHM server results don't match";

    // Calculate and print performance metrics
    std::chrono::duration<double> sync_duration = sync_end_time - start_time;
    // std::chrono::duration<double> async_duration = async_end_time - async_start_time;
    // std::chrono::duration<double> shm_duration = shm_end_time - shm_start_time;

    std::cout << "Performance comparison:" << std::endl;
    std::cout << "  Sync server: " << sync_duration.count() << " seconds" << std::endl;
    // std::cout << "  Async server: " << async_duration.count() << " seconds" << std::endl;
    // std::cout << "  Async SHM server: " << shm_duration.count() << " seconds" << std::endl;

    // Calculate speedup compared to sync server
    // double async_speedup = sync_duration.count() / async_duration.count();
    // double shm_speedup = sync_duration.count() / shm_duration.count();

    std::cout << "Speedup compared to sync server:" << std::endl;
    // std::cout << "  Async server: " << async_speedup << "x" << std::endl;
    // std::cout << "  Async SHM server: " << shm_speedup << "x" << std::endl;
}

// Test with larger queries to stress the distributed system
TEST_F(DistributedServerTest, LargeQueryScaling) {
    // Test with Manhattan borough which should return many records
    Query query = createManhattanQuery();

    // Query each server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> sync_results = queryServer(SYNC_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> sync_duration = end_time - start_time;

    // Make sure we got results
    ASSERT_FALSE(sync_results.empty()) << "Sync server returned no results for Manhattan query";

    // Verify that all results are from Manhattan
    EXPECT_TRUE(verifyManhattanCollisions(sync_results))
        << "Sync server returned records that don't match Manhattan criteria";

    // Log basic info about results
    std::cout << "Manhattan query returned " << sync_results.size()
              << " collisions in " << sync_duration.count() << " seconds" << std::endl;

    // Calculate and print throughput metrics
    double records_per_second = sync_results.size() / sync_duration.count();
    std::cout << "Throughput: " << records_per_second << " records/second" << std::endl;

    // Now test async servers only if result count is manageable
    // if (sync_results.size() < 10000) {
    //     std::this_thread::sleep_for(std::chrono::seconds(2));

    //     auto async_start_time = std::chrono::high_resolution_clock::now();
    //     std::vector<Collision> async_results = queryServer(ASYNC_BASE_PORT, query);
    //     auto async_end_time = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double> async_duration = async_end_time - async_start_time;

    //     ASSERT_FALSE(async_results.empty()) << "Async server returned no results for Manhattan query";

    //     std::this_thread::sleep_for(std::chrono::seconds(2));

    //     auto shm_start_time = std::chrono::high_resolution_clock::now();
    //     std::vector<Collision> async_shm_results = queryServer(ASYNC_SHM_BASE_PORT, query);
    //     auto shm_end_time = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double> shm_duration = shm_end_time - shm_start_time;

    //     ASSERT_FALSE(async_shm_results.empty()) << "Async SHM server returned no results for Manhattan query";

    //     // Compare result counts - they should be the same
    //     EXPECT_EQ(sync_results.size(), async_results.size())
    //         << "Sync and async server returned different number of results";
    //     EXPECT_EQ(sync_results.size(), async_shm_results.size())
    //         << "Sync and async SHM server returned different number of results";

    //     // Calculate and print throughput metrics for all servers
    //     double async_records_per_second = async_results.size() / async_duration.count();
    //     double shm_records_per_second = async_shm_results.size() / shm_duration.count();

    //     std::cout << "Throughput comparison:" << std::endl;
    //     std::cout << "  Sync server: " << records_per_second << " records/second" << std::endl;
    //     std::cout << "  Async server: " << async_records_per_second << " records/second" << std::endl;
    //     std::cout << "  Async SHM server: " << shm_records_per_second << " records/second" << std::endl;
    // } else {
    //     std::cout << "Skipping async server tests due to large result set" << std::endl;
    // }
}

// Test the robustness of duplicate detection in Process E (rank 4)
// TEST_F(DistributedServerTest, DuplicateQueryDetection) {
//     // Create a fixed ID query to test duplicate detection
//     Query query = createBrooklynQuery();

//     // We'll test with the sync server since it has the clearest duplicate detection code
//     std::string target_str = "127.0.0.1:" + std::to_string(SYNC_BASE_PORT);
//     std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
//         target_str, grpc::InsecureChannelCredentials());
//     std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub =
//         collision_proto::CollisionQueryService::NewStub(channel);

//     // Create a query with a fixed ID to ensure it's recognized as duplicate
//     const uint32_t FIXED_QUERY_ID = 12345;

//     QueryRequest query_request = {
//         .id = FIXED_QUERY_ID,
//         .requested_by = {std::numeric_limits<std::uint32_t>::max()},
//         .query = query,
//     };

//     collision_proto::QueryRequest request = QueryProtoConverter::serialize(query_request);

//     // First query - should succeed
//     {
//         collision_proto::QueryResponse response;
//         grpc::ClientContext context;
//         auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(RPC_TIMEOUT);
//         context.set_deadline(deadline);

//         std::cout << "Sending first query with ID " << FIXED_QUERY_ID << std::endl;
//         grpc::Status status = stub->GetCollisions(&context, request, &response);

//         EXPECT_TRUE(status.ok()) << "First query failed: " << status.error_message();

//         if (status.ok()) {
//             auto result = CollisionProtoConverter::deserialize(response);
//             std::cout << "First query returned " << result.collisions.size() << " results" << std::endl;
//             EXPECT_FALSE(result.collisions.empty()) << "First query returned no results";
//         }
//     }

//     // Wait a moment to ensure the first query is processed
//     std::this_thread::sleep_for(std::chrono::seconds(1));

//     // Second query with same ID - should still succeed at the client level
//     // but internally Process E should detect and reject the duplicate
//     {
//         collision_proto::QueryResponse response;
//         grpc::ClientContext context;
//         auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(RPC_TIMEOUT);
//         context.set_deadline(deadline);

//         std::cout << "Sending duplicate query with ID " << FIXED_QUERY_ID << std::endl;
//         grpc::Status status = stub->GetCollisions(&context, request, &response);

//         // The client should still receive an OK status since Process A aggregates results
//         // even if some processes (like E) reject the duplicate
//         EXPECT_TRUE(status.ok()) << "Duplicate query failed: " << status.error_message();

//         if (status.ok()) {
//             auto result = CollisionProtoConverter::deserialize(response);
//             std::cout << "Duplicate query returned " << result.collisions.size() << " results" << std::endl;

//             // We expect results from some processes, just potentially not from Process E
//             // So we can't make strong assertions about the result count
//         }
//     }
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}