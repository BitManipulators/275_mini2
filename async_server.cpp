#include "collision_query_service_impl.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "ring_buffer.hpp"
#include "yaml_parser.hpp"
#include "myconfig.hpp"

#include "collision_manager/collision_manager.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>


const std::string CSV_FILE = std::string("../Motor_Vehicle_Collisions_-_Crashes_20250123.csv");
static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(CSV_FILE);

std::uint32_t rank;

std::mutex pendingRequestsMutex{};
PendingRequestsRingbuffer pendingRequests{};
std::condition_variable pendingRequestsConditionVariable{};

std::mutex pendingResponsesMutex{};
PendingResponsesRingbuffer pendingResponses{};
std::condition_variable pendingResponsesConditionVariable{};

std::vector<std::thread> requestWorkers{};
std::vector<std::thread> responseWorkers{};

std::unordered_map<std::size_t, GetCollisionsClientRequest> pendingClientRequestsMap{};


grpc::Status queryPeer(const std::string peer_address, const QueryRequest& query_request)
{
    collision_proto::QueryRequest request = QueryProtoConverter::serialize(query_request);

    grpc::Status status = Status(grpc::StatusCode::UNKNOWN, "UNKNOWN");
    try {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials());
        std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

        Empty response;
        grpc::ClientContext clientContext;

        // Set a deadline (e.g., 2 seconds from now)
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
        clientContext.set_deadline(deadline);

        status = stub->SendRequest(&clientContext, request, &response);

        if (!status.ok())
        {
            std::cerr << "Error querying peer: " << status.error_message() << std::endl;
        }
    } catch (...) {
        std::cout << "Hmm " << std::endl;
    }

    return status;
}

grpc::Status sendResults(const std::string peer_address, const QueryResponse& query_response)
{
    collision_proto::QueryResponse response = CollisionProtoConverter::serialize(query_response);

    grpc::Status status = Status(grpc::StatusCode::UNKNOWN, "UNKNOWN");
    try {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials());
        std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

        Empty empty;
        grpc::ClientContext clientContext;

        // Set a deadline (e.g., 2 seconds from now)
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
        clientContext.set_deadline(deadline);

        status = stub->ReceiveResponse(&clientContext, response, &empty);
        if (!status.ok())
        {
            std::cerr << "Error sending results: " << status.error_message() << std::endl;
        }
    } catch (...) {
        std::cout << "Hmm " << std::endl;
    }

    return status;
}

QueryRequest wait_for_new_query_request(std::uint32_t id) {
    std::unique_lock<std::mutex> lock(pendingRequestsMutex);
    pendingRequestsConditionVariable.wait(lock, []() { return !pendingRequests.is_empty(); });

    QueryRequest&& query_request = pendingRequests.pop();
    return query_request;
}

QueryResponse wait_for_new_query_response(std::uint32_t id) {
    std::unique_lock<std::mutex> lock(pendingResponsesMutex);
    pendingResponsesConditionVariable.wait(lock, []() { return !pendingResponses.is_empty(); });

    QueryResponse&& query_response = pendingResponses.pop();
    return query_response;
}

void handle_pending_requests(std::uint32_t id, std::uint32_t rank) {
    while (true) {
        QueryRequest query_request = wait_for_new_query_request(id);
        query_request.requested_by.push_back(rank);

        std::cout << "RequestWorker " << id << " is handling query: " << query_request.id << std::endl;

        QueryResponse query_response = {
            .id = query_request.id,
            .requested_by = query_request.requested_by,
            .results_from = rank,
            .collisions = collision_manager->search(query_request.query),
        };

        {
            std::lock_guard<std::mutex> lock(pendingResponsesMutex);
            pendingResponses.push(query_response);
            pendingResponsesConditionVariable.notify_one();

            std::cout << "Added response from: '" << static_cast<char>('A' + query_response.results_from) <<
                         "' with id: '" << query_response.id << "' to the pendingResponses ringbuffer" << std::endl;
        }

        // TODO: This should just iterate over each children() or something.

        grpc::Status status;
        // Query based on overlay:
        if (rank == 0) {
            // Process A: query only Process B.
            status = queryPeer("127.0.0.1:50052", query_request);
            if (!status.ok()) {
                // TODO
            }
        }
        else if (rank == 1)
        {
            // Process B: query Processes C and D.
            status = queryPeer("127.0.0.1:50053", query_request);
            if (!status.ok()) {
                // TODO
            }

            status = queryPeer("127.0.0.1:50054", query_request);
            if (!status.ok()) {
                // TODO
            }
        }
        else if (rank == 2)
        {
            // Process C: query Process E.
            status = queryPeer("127.0.0.1:50055", query_request);
            if (!status.ok()) {
                // TODO
            }
        }
        else if (rank == 3)
        {
            // Process D: query Process E.
            status = queryPeer("127.0.0.1:50055", query_request);
            if (!status.ok()) {
                // TODO
            }
        }
        // Process E (rank 4) does not forward the query further.

        std::cout << "RequestWorker " << id << " has processed query: " << query_request.id << std::endl;
    }
}

void handle_client_pending_responses(std::uint32_t worker_id, std::uint32_t process_rank, const QueryResponse& query_response) {
    auto map_it = pendingClientRequestsMap.find(query_response.id);

    if (map_it == pendingClientRequestsMap.end()) {
        std::cerr << "ResponseWorker " << worker_id << " could not find client_request for id: " << query_response.id << std::endl;
        return;
    }

    GetCollisionsClientRequest& client_request = map_it->second;

    auto rank_it = std::find(client_request.ranks.begin(), client_request.ranks.end(), query_response.results_from);

    if (rank_it != client_request.ranks.end()) {
        std::cout << "ResponseWorker " << worker_id << " sees rank has already been processed for client_request with id: "
                  << query_response.id << std::endl;
        return;
    }

    client_request.collisions.insert(client_request.collisions.end(), query_response.collisions.begin(), query_response.collisions.end());
    client_request.ranks.push_back(query_response.results_from);

    // TODO: check if all ranks present better using yaml config to find out how many processes exist
    if (client_request.ranks.size() == 5) {
        GetCollisionsCallData* getCollisionsCallData = dynamic_cast<GetCollisionsCallData*>(client_request.call_data_base);
        if (getCollisionsCallData == nullptr) {
            std::cerr << "ResponseWorker " << worker_id << " could not convert call_data_base for client_request with id: "
                  << query_response.id << std::endl;
            return;
        }

        getCollisionsCallData->CompleteRequest(query_response.id, client_request.collisions);
        pendingClientRequestsMap.erase(map_it);
    }
}

void handle_pending_responses(std::uint32_t worker_id, std::uint32_t process_rank) {
    while (true) {
        QueryResponse query_response = wait_for_new_query_response(worker_id);

        std::cout << "ResponseWorker " << worker_id << " is handling query: " << query_response.id << std::endl;

        if (process_rank == 0) {
            handle_client_pending_responses(worker_id, process_rank, query_response);
            continue;
        }

        std::uint32_t parent_rank = *(query_response.requested_by.end() - 2);
        int parent_port = 50051 + parent_rank;
        std::string parent_server_address = "127.0.0.1:" + std::to_string(parent_port);

        std::cout << "Parent server address is: " << parent_server_address << std::endl;

        grpc::Status status = sendResults(parent_server_address, query_response);

        std::cout << "ResponseWorker " << worker_id << " has processed query: " << query_response.id << std::endl;
    }
}

int main(int argc, char** argv) {
    
    /*DeploymentConfig config = parseConfig("../config.yaml");

    const char *rankEnv = std::getenv("RANK");
    if (!rankEnv) {
        std::cerr << "RANK environment variable not set!" << std::endl;
        return 1;
    }
    rank = std::stoi(rankEnv);
    std::string selfProcessId = "process_" + std::string(1, 'a' + rank);
    std::cout << "Self process id: " << selfProcessId << std::endl;

    // Build peer addresses by excluding self.
    std::vector<std::string> peerAddresses = getPeerAddresses(config, selfProcessId);
    std::cout << "Peer addresses:" << std::endl;
    for (const auto &addr : peerAddresses) {
        std::cout << "  " << addr << std::endl;
    }*/

    MyConfig*  myconfig = MyConfig::getInstance();

    rank = myconfig->getRank();

    CollisionQueryServiceImpl service{rank,
                                      pendingRequestsMutex,
                                      pendingResponsesMutex,
                                      pendingRequests,
                                      pendingResponses,
                                      pendingRequestsConditionVariable,
                                      pendingResponsesConditionVariable,
                                      pendingClientRequestsMap};

    for (int i = 0; i < 4; ++i) {
        requestWorkers.push_back(std::thread(handle_pending_requests, i, rank));
    }

    for (int i = 0; i < 4; ++i) {
        responseWorkers.push_back(std::thread(handle_pending_responses, i, rank));
    }

    int port = 50051 + rank; // 50051, 50052, 50053, etc.

    service.Run(port);

    for (auto& worker : requestWorkers) {
        worker.join();
    }

    for (auto& worker : responseWorkers) {
        worker.join();
    }

    return 0;
}
