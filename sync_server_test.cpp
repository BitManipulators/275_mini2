#include "server_test_utils.hpp"

// Base port for sync server
const int SYNC_BASE_PORT = 50051;

class SyncServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

// // Test basic query functionality
TEST_F(SyncServerTest, BrooklynZipQueryTest) {

    Query query = ServerTestUtils::createBrooklynQuery();

    // Query the sync server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> results = ServerTestUtils::queryServer(SYNC_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    ASSERT_FALSE(results.empty()) << "Sync server returned no results";

    EXPECT_TRUE(ServerTestUtils::verifyBrooklynCollisions(results))
        << "Sync server returned records that don't match query criteria";

    ServerTestUtils::logResults("Sync Server", results);

    std::cout << "Query completed in " << duration.count() << " seconds" << std::endl;
    std::cout << "Result count: " << results.size() << std::endl;
    std::cout << "Throughput: " << results.size() / duration.count() << " records/second" << std::endl;
}

// // Test for duplicate query detection
// TEST_F(SyncServerTest, DuplicateQueryDetectionTest) {
//     // Create a fixed ID query to test duplicate detection
//     Query query = ServerTestUtils::createBrooklynQuery();

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

//     // First query should succeed
//     {
//         collision_proto::QueryResponse response;
//         grpc::ClientContext context;
//         auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(ServerTestUtils::RPC_TIMEOUT);
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

//     // Second query with same ID should still succeed at the client level
//     // but internally Process E should detect and reject the duplicate
//     {
//         collision_proto::QueryResponse response;
//         grpc::ClientContext context;
//         auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(ServerTestUtils::RPC_TIMEOUT);
//         context.set_deadline(deadline);

//         std::cout << "Sending duplicate query with ID " << FIXED_QUERY_ID << std::endl;
//         grpc::Status status = stub->GetCollisions(&context, request, &response);

//         EXPECT_TRUE(status.ok()) << "Duplicate query failed: " << status.error_message();

//         if (status.ok()) {
//             auto result = CollisionProtoConverter::deserialize(response);
//             std::cout << "Duplicate query returned " << result.collisions.size() << " results" << std::endl;
//         }
//     }
// }

TEST_F(SyncServerTest, ManhattanQueryTest) {
    // Test with Manhattan borough which should return many records
    Query query = ServerTestUtils::createManhattanQuery();

    // Query the sync server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> results = ServerTestUtils::queryServer(SYNC_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    // Make sure we got results
    ASSERT_FALSE(results.empty()) << "Sync server returned no results for Manhattan query";

    // Verify that all results are from Manhattan
    EXPECT_TRUE(ServerTestUtils::verifyManhattanCollisions(results))
        << "Sync server returned records that don't match Manhattan criteria";

    // Log basic info about results
    std::cout << "Manhattan query returned " << results.size()
              << " collisions in " << duration.count() << " seconds" << std::endl;

    // Calculate and print throughput metrics
    double records_per_second = results.size() / duration.count();
    std::cout << "Throughput: " << records_per_second << " records/second" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}