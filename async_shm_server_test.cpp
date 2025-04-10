#include "server_test_utils.hpp"

// Base port for async SHM server
const int ASYNC_SHM_BASE_PORT = 50051;

class AsyncShmServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

TEST_F(AsyncShmServerTest, BrooklynZipQueryTest) {
    Query query = ServerTestUtils::createBrooklynQuery();

    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Collision> results = ServerTestUtils::queryServer(ASYNC_SHM_BASE_PORT, query);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    ASSERT_FALSE(results.empty()) << "Async SHM server returned no results";

    EXPECT_TRUE(ServerTestUtils::verifyBrooklynCollisions(results))
        << "Async SHM server returned records that don't match query criteria";

    ServerTestUtils::logResults("Async SHM Server", results);

    std::cout << "Query completed in " << duration.count() << " seconds" << std::endl;
    std::cout << "Result count: " << results.size() << std::endl;
    std::cout << "Throughput: " << results.size() / duration.count() << " records/second" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}