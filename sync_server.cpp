#include "collision_manager/collision_manager.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "yaml_parser.hpp"
#include <omp.h>
#include <grpcpp/grpcpp.h>
#include <future>
#include <filesystem>
#include <iostream>


const std::string CSV_FILE = std::string("../Motor_Vehicle_Collisions_-_Crashes_20250123.csv");

static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(CSV_FILE);

std::mutex queryMutex;
std::unordered_set<std::string> processedQueries;

// Compute a hash for the query using its DebugString
std::string computeQueryHash(const collision_proto::QueryRequest& request) {
    std::string queryStr = request.DebugString(); // Use DebugString to serialize the query
    std::hash<std::string> hasher;
    size_t hashValue = hasher(queryStr);
    return std::to_string(hashValue);
}

QueryResponse queryPeer(const std::string peer_address, const collision_proto::QueryRequest &request)
{
    std::vector<collision_proto::Collision> peerCollisions;
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials());
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    collision_proto::QueryResponse peerResponse;
    grpc::ClientContext clientContext;

        // Set a deadline (e.g., 2 seconds from now)
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    clientContext.set_deadline(deadline);

    grpc::Status status = stub->GetCollisions(&clientContext, request, &peerResponse);

    if (status.ok())
    {
        QueryResponse query_response = CollisionProtoConverter::deserialize(peerResponse);
        return query_response;
    }
    else
    {
        std::cerr << "Error querying peer: " << status.error_message() << std::endl;
        return {};
    }
}

class CollisionQueryServiceImpl final : public collision_proto::CollisionQueryService::Service {
public:
    std::vector<std::string> peer_addresses_;

CollisionQueryServiceImpl(const std::vector<std::string>& peers) : peer_addresses_(peers) {}

grpc::Status GetCollisions(grpc::ServerContext* context,
                           const collision_proto::QueryRequest* request,
                           collision_proto::QueryResponse* response) override {

    // Get the rank of the current process.
    const char* rankEnv = std::getenv("RANK");
    int rank = rankEnv ? std::stoi(rankEnv) : 0;

    // For Process E (rank 4), check for duplicate queries.
    if (rank == 4) {
        std::string queryHash = computeQueryHash(*request);
        {
            std::lock_guard<std::mutex> lock(queryMutex);
            if (processedQueries.find(queryHash) != processedQueries.end()) {
                std::cerr << "Duplicate query detected in Process E (hash: " << queryHash << "). Rejecting query." << std::endl;
                return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Query already processed");
            }
            // Mark query as processed.
            processedQueries.insert(queryHash);
        }
    }

    // Run the local query.
    QueryRequest query_request = QueryProtoConverter::deserialize(*request);
    std::vector<Collision> localResults = collision_manager->search(query_request.query);

    std::vector<Collision> aggregatedResults{};
    aggregatedResults.insert(aggregatedResults.end(), localResults.begin(), localResults.end());

    // Query based on overlay:
    if (rank == 0) {
        // Process A: query only Process B.
        QueryResponse resultsB = queryPeer("127.0.0.1:50052", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsB.collisions.begin(), resultsB.collisions.end());
        std::cout << "Results from B: " << resultsB.collisions.size() << std::endl;
    }
    else if (rank == 1) {
        // Process B: query Processes C and D.
        QueryResponse resultsC = queryPeer("127.0.0.1:50053", *request);
        QueryResponse resultsD = queryPeer("127.0.0.1:50054", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsC.collisions.begin(), resultsC.collisions.end());
        aggregatedResults.insert(aggregatedResults.end(), resultsD.collisions.begin(), resultsD.collisions.end());
        std::cout << "Results from C: " << resultsC.collisions.size() << std::endl;
        std::cout << "Results from D: " << resultsD.collisions.size() << std::endl;
    }
    else if (rank == 2) {
        // Process C: query Process E.
        QueryResponse resultsE = queryPeer("127.0.0.1:50055", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsE.collisions.begin(), resultsE.collisions.end());
        std::cout << "Results from E: " << resultsE.collisions.size() << std::endl;
    }
    else if (rank == 3) {
        // Process D: query Process E.
        QueryResponse resultsE = queryPeer("127.0.0.1:50055", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsE.collisions.begin(), resultsE.collisions.end());
        std::cout << "Results from E: " << resultsE.collisions.size() << std::endl;
    }
    // Process E (rank 4) does not forward the query further.

    // The coordinator (process A) prints the aggregated size.
    if (rank == 0) {
        std::cout << "Aggregated collision size: " << aggregatedResults.size() << std::endl;
    }

    // Build final response.
    QueryResponse query_response = {
        .id = query_request.id,
        .requested_by = {},
        .results_from = static_cast<std::uint32_t>(rank),
        .collisions = aggregatedResults,
    };

    CollisionProtoConverter::serialize(query_response, *response);

    return grpc::Status::OK;
}


};


void RunServer(const std::vector<std::string>& peerAddresses) {

    const char* rankEnv = std::getenv("RANK");
    int rank = rankEnv ? std::stoi(rankEnv) : 0;
    int port = 50051 + rank; // 50051, 50052, 50053, etc.

    std::string server_address = "127.0.0.1:" + std::to_string(port);
   // std::string server_address("0.0.0.0:50051");
    CollisionQueryServiceImpl service(peerAddresses);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server Listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char *argv[]) {
    DeploymentConfig config = parseConfig("../config.yaml");

    const char *rankEnv = std::getenv("RANK");
    if (!rankEnv) {
        std::cerr << "RANK environment variable not set!" << std::endl;
        return 1;
    }
    int rank = std::stoi(rankEnv);
    std::string selfProcessId = "process_" + std::string(1, 'a' + rank);
    std::cout << "Self process id: " << selfProcessId << std::endl;

    // Build peer addresses by excluding self.
    std::vector<std::string> peerAddresses = getPeerAddresses(config, selfProcessId);
    std::cout << "Peer addresses:" << std::endl;
    for (const auto &addr : peerAddresses) {
        std::cout << "  " << addr << std::endl;
    }

    RunServer(peerAddresses);
    return 0;
}
