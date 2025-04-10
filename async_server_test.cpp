#include "server_test_utils.hpp"

// Base port for async server
const int ASYNC_BASE_PORT = 50051;

class AsyncServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

TEST_F(AsyncServerTest, BrooklynZipQueryTest) {
    Query query = ServerTestUtils::createBrooklynQuery();

    // Query the async server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> results = ServerTestUtils::queryServer(ASYNC_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    ASSERT_FALSE(results.empty()) << "Async server returned no results";

    EXPECT_TRUE(ServerTestUtils::verifyBrooklynCollisions(results))
        << "Async server returned records that don't match query criteria";

    ServerTestUtils::logResults("Async Server", results);

    std::cout << "Query completed in " << duration.count() << " seconds" << std::endl;
    std::cout << "Result count: " << results.size() << std::endl;
    std::cout << "Throughput: " << results.size() / duration.count() << " records/second" << std::endl;
}

TEST_F(AsyncServerTest, ManhattanQueryTest) {
    // Test with Manhattan borough which should return many records
    Query query = ServerTestUtils::createManhattanQuery();

    // Query the Async server
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> results = ServerTestUtils::queryServer(ASYNC_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    // Make sure we got results
    ASSERT_FALSE(results.empty()) << "ASync server returned no results for Manhattan query";

    // Verify that all results are from Manhattan
    EXPECT_TRUE(ServerTestUtils::verifyManhattanCollisions(results))
        << "ASync server returned records that don't match Manhattan criteria";

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