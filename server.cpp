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

std::vector<collision_proto::Collision> queryPeer(const std::string peer_address, const collision_proto::QueryRequest &request)
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
        for (const auto &collision : peerResponse.collision())
        {
            peerCollisions.push_back(collision);
        }
        return peerCollisions;
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

//     grpc::Status GetCollisions(grpc::ServerContext* context,
//                            const collision_proto::QueryRequest* request,
//                            collision_proto::QueryResponse* response) override {

//     Query query = QueryProtoConverter::deserialize(request);
//     std::vector<CollisionProxy*> localCollisions = collision_manager->searchOpenMp(query);

//     std::vector<collision_proto::Collision> localResults;
//     for (auto proxy : localCollisions) {
//         collision_proto::Collision temp;
//         CollisionProtoConverter::serialize(proxy, &temp);
//         localResults.push_back(temp);
//     }

//     std::cout << "Local collision size: " << localResults.size() << std::endl;

//     std::vector<collision_proto::Collision> aggregatedResults = localResults;

//     const char* rankEnv = std::getenv("RANK");
//     int rank = rankEnv ? std::stoi(rankEnv) : 0;
//     if (rank == 0) {
//         for (const auto &addr : peer_addresses_) {
//             std::vector<collision_proto::Collision> peerCollisions = queryPeer(addr, *request);
//             std::cout << "Received " << peerCollisions.size() << " collisions from peer " << addr << std::endl;
//             aggregatedResults.insert(aggregatedResults.end(), peerCollisions.begin(), peerCollisions.end());
//         }
//         std::cout << "Aggregated collision size: " << aggregatedResults.size() << std::endl;
//     }

//     for (const auto &collision : aggregatedResults) {
//         collision_proto::Collision* newcollision = response->add_collision();
//         newcollision->CopyFrom(collision);
//     }

//     return grpc::Status::OK;
// }

grpc::Status GetCollisions(grpc::ServerContext* context,
                           const collision_proto::QueryRequest* request,
                           collision_proto::QueryResponse* response) override {
    // Run the local query.
    Query query = QueryProtoConverter::deserialize(request);
    std::vector<CollisionProxy*> localResults = collision_manager->searchOpenMp(query);

    std::vector<collision_proto::Collision> aggregatedResults;
    for (auto proxy : localResults) {
        collision_proto::Collision temp;
        CollisionProtoConverter::serialize(proxy, &temp);
        aggregatedResults.push_back(temp);
    }

    // Determine process identity
    const char* rankEnv = std::getenv("RANK");
    int rank = rankEnv ? std::stoi(rankEnv) : 0;

    // Query based on overlay:
    if (rank == 0) {
        // Process A: query only Process B.
        std::vector<collision_proto::Collision> resultsB = queryPeer("127.0.0.1:50052", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsB.begin(), resultsB.end());
        std::cout << "Results from B: " << resultsB.size() << std::endl;
    }
    else if (rank == 1) {
        // Process B: query Processes C and D.
        std::vector<collision_proto::Collision> resultsC = queryPeer("127.0.0.1:50053", *request);
        std::vector<collision_proto::Collision> resultsD = queryPeer("127.0.0.1:50054", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsC.begin(), resultsC.end());
        aggregatedResults.insert(aggregatedResults.end(), resultsD.begin(), resultsD.end());
        std::cout << "Results from C: " << resultsC.size() << std::endl;
        std::cout << "Results from D: " << resultsD.size() << std::endl;
    }
    else if (rank == 2) {
        // Process C: query Process E.
        std::vector<collision_proto::Collision> resultsE = queryPeer("127.0.0.1:50055", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsE.begin(), resultsE.end());
        std::cout << "Results from E: " << resultsE.size() << std::endl;
    }
    else if (rank == 3) {
        // Process D: query Process E.
        std::vector<collision_proto::Collision> resultsE = queryPeer("127.0.0.1:50055", *request);
        aggregatedResults.insert(aggregatedResults.end(), resultsE.begin(), resultsE.end());
        std::cout << "Results from E: " << resultsE.size() << std::endl;
    }
    // Process E (rank 4) does not forward the query further.

    // The coordinator (process A) prints the aggregated size.
    if (rank == 0) {
        std::cout << "Aggregated collision size: " << aggregatedResults.size() << std::endl;
    }

    // Build final response.
    for (const auto &collision : aggregatedResults) {
        collision_proto::Collision* newcollision = response->add_collision();
        newcollision->CopyFrom(collision);
    }

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
