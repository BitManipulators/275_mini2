#include "collision_query_service_impl.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "ring_buffer.hpp"
#include "shared_memory_manager.hpp"
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


const std::string CSV_FILE = std::string("/home/suriya-018231499/cpp_projects/parser_data/Motor_Vehicle_Collisions_-_Crashes_20250204.csv");
static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(CSV_FILE);

std::mutex pendingRequestsMutex{};
std::mutex pendingClientRequestsMutex{};
std::mutex pendingResponsesMutex{};

PendingRequestsRingbuffer pendingRequests{};
PendingResponsesRingbuffer pendingResponses{};

std::condition_variable pendingRequestsConditionVariable{};
std::condition_variable pendingResponsesConditionVariable{};

std::atomic<bool> worker_stop_flag(false);

std::vector<std::thread> requestWorkers{};
std::vector<std::thread> responseWorkers{};
std::vector<std::thread> shmResponseWorkers{};

std::unordered_map<std::size_t, GetCollisionsClientRequest> pendingClientRequestsMap{};

SharedMemoryManager* shared_memory_manager = nullptr;

std::uint32_t rank;
Config config;
MyConfig*  myconfig = MyConfig::getInstance();
//std::vector<std::size_t> child_ranks{};
//std::vector<std::size_t> ranks_on_same_machine{};

//bool is_rank_on_same_machine(const std::size_t rank) {
//    return std::find(ranks_on_same_machine.begin(), ranks_on_same_machine.end(), rank) != ranks_on_same_machine.end();
//}

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

std::optional<QueryRequest> wait_for_new_query_request(std::uint32_t id) {
    std::unique_lock<std::mutex> lock(pendingRequestsMutex);
    pendingRequestsConditionVariable.wait(lock, []() { return !pendingRequests.is_empty() || worker_stop_flag.load(); });

    if (worker_stop_flag.load()) {
        std::cout << "RequestWorker: " << id << " stopping due to stop flag set." << std::endl;
        return {};
    }

    return {pendingRequests.pop()};
}

std::optional<QueryResponse> wait_for_new_query_response(std::uint32_t id) {
    std::unique_lock<std::mutex> lock(pendingResponsesMutex);
    pendingResponsesConditionVariable.wait(lock, []() { return !pendingResponses.is_empty() || worker_stop_flag.load(); });

    if (worker_stop_flag.load()) {
        std::cout << "ResponseWorker: " << id << " stopping due to stop flag set." << std::endl;
        return {};
    }

    return {pendingResponses.pop()};
}

std::optional<SharedMemoryQueryResponse> wait_for_new_shared_memory_query_response(std::uint32_t id) {
    while (!worker_stop_flag.load()) {
        if (!shared_memory_manager->has_results()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        return {shared_memory_manager->pop_result()};
    }

    return {};
}

void handle_pending_requests(std::uint32_t worker_id, std::uint32_t rank) {
    
   
    while (!worker_stop_flag.load()) {
        std::optional<QueryRequest> maybe_query_request = wait_for_new_query_request(worker_id);
        if (!maybe_query_request.has_value()) {
            continue;
        }
        QueryRequest& query_request = maybe_query_request.value();

        query_request.requested_by.push_back(rank);

        std::cout << "RequestWorker " << worker_id << " is handling query: " << query_request.id << std::endl;

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

        for (const auto& neighbour : myconfig->getLogicalNeighbors()){
            grpc::Status status;

            //int child_port = 50051 + child_rank;
            //std::string child_server_address = "127.0.0.1:" + std::to_string(child_port);

            std::cout << "Child server address is: " << neighbour << std::endl;

            status = queryPeer(neighbour, query_request);
            if (!status.ok()) {
                // TODO
            }
        }

        std::cout << "RequestWorker " << worker_id << " has processed query: " << query_request.id << std::endl;
    }

    std::cout << "RequestWorker: " << worker_id << " stopped." << std::endl;
}

void handle_client_pending_responses(std::uint32_t worker_id, std::uint32_t process_rank, const QueryResponse& query_response) {
    
    
    std::unique_lock<std::mutex> lock(pendingClientRequestsMutex);

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
    if (client_request.ranks.size() == myconfig->getTotalNumberofProcess()) {
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
    
    while (!worker_stop_flag.load()) {
        std::optional<QueryResponse> maybe_query_response = wait_for_new_query_response(worker_id);
        if (!maybe_query_response.has_value()) {
            continue;
        }
        QueryResponse& query_response = maybe_query_response.value();

        std::cout << "ResponseWorker " << worker_id << " is handling response: " << query_response.id << std::endl;

        if (process_rank == 0) {
            handle_client_pending_responses(worker_id, process_rank, query_response);
            continue;
        }

        
        std::uint32_t parent_rank = *(query_response.requested_by.end() - 2);
        if (myconfig->isSameNodeProcess(parent_rank)) {
            shared_memory_manager->send_results(parent_rank, query_response);
        } else {
            //int parent_port = 50051 + parent_rank;
            //std::string parent_server_address = "127.0.0.1:" + std::to_string(parent_port);

            std::string parent_server_address = config.getaddress(parent_rank);
            std::cout << "Parent server address is: " << parent_server_address << std::endl;

            grpc::Status status = sendResults(parent_server_address, query_response);
        }

        std::cout << "ResponseWorker " << worker_id << " has processed response: " << query_response.id << std::endl;
    }

    std::cout << "ResponseWorker: " << worker_id << " stopped." << std::endl;
}

void handle_shared_memory_pending_responses(std::uint32_t worker_id, std::uint32_t process_rank) {
    
    
    while (!worker_stop_flag.load()) {
        std::optional<SharedMemoryQueryResponse> maybe_query_response = wait_for_new_shared_memory_query_response(worker_id);
        if (!maybe_query_response.has_value()) {
            continue;
        }
        SharedMemoryQueryResponse& shared_memory_query_response = maybe_query_response.value();

        std::cout << "SharedMemoryResponseWorker " << worker_id << " is handling response: " << shared_memory_query_response.id << std::endl;

        std::uint32_t parent_rank = *(shared_memory_query_response.requested_by.begin() + shared_memory_query_response.requested_by_size - 2);
        if (process_rank != 0 && myconfig->isSameNodeProcess(parent_rank)) {
            shared_memory_manager->send_results(parent_rank, shared_memory_query_response);
        } else {
            // convert to QueryResponse and push it to self
            QueryResponse query_response = shared_memory_manager->deserialize(shared_memory_query_response);
            {
                std::lock_guard<std::mutex> lock(pendingResponsesMutex);
                pendingResponses.push(query_response);
                pendingResponsesConditionVariable.notify_one();

                std::cout << "SharedMemoryResponseWorker added response from: '" << static_cast<char>('A' + query_response.results_from) <<
                             "' with id: '" << query_response.id << "' to the pendingResponses ringbuffer" << std::endl;
            }
        }

        std::cout << "SharedMemoryResponseWorker " << worker_id << " has processed response from: " << shared_memory_query_response.results_from
                  << " with id: " << shared_memory_query_response.id << std::endl;
    }

    std::cout << "SharedMemoryResponseWorker: " << worker_id << " stopped." << std::endl;
}

void cleanup_worker_threads() {
    worker_stop_flag.store(true);
    pendingRequestsConditionVariable.notify_all();
    pendingResponsesConditionVariable.notify_all();

    std::cout << "Joining request worker threads" << std::endl;
    for (auto& worker : requestWorkers) {
        worker.join();
    }

    std::cout << "Joining response worker threads" << std::endl;
    for (auto& worker : responseWorkers) {
        worker.join();
    }

    std::cout << "Joining shm response worker threads" << std::endl;
    for (auto& worker : shmResponseWorkers) {
        worker.join();
    }
}

void cleanup_shared_memory() {
    if (shared_memory_manager != nullptr) {
        SharedMemoryManager* temp = shared_memory_manager;
        shared_memory_manager = nullptr;
        delete temp;
    }
}

sigset_t old_mask;
void handle_signal(int signal) {
    sigset_t new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, signal);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    if (signal == SIGINT) {
        std::cerr << "Ctrl+C pressed, handling SIGINT..." << std::endl;
    } else if (signal == SIGABRT) {
        std::cerr << "Aborting, handling SIGABRT..." << std::endl;
    } else if (signal == SIGSEGV) {
        std::cerr << "Segmentation fault, handling SIGSEGV..." << std::endl;
    }

    cleanup_worker_threads();
    cleanup_shared_memory();

    exit(signal);

    sigprocmask(SIG_SETMASK, &old_mask, nullptr);
}

int main(int argc, char** argv) {
    signal(SIGINT, handle_signal);
//    signal(SIGABRT, handle_signal);
//    signal(SIGSEGV, handle_signal);

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
    } */

    
    rank = myconfig->getRank();

    // TODO: Use config.yaml
    //ranks_on_same_machine = {0, 1, 2, 3, 4};

    /*if (rank == 0) {
        // Process A: query only Process B.
        child_ranks = {1};
    }
    else if (rank == 1)
    {
        // Process B: query Processes C and D.
        child_ranks = {2, 3};
    }
    else if (rank == 2)
    {
        // Process C: query Process E.
        child_ranks = {4};
    }
    else if (rank == 3)
    {
        // Process D: query Process E.
        child_ranks = {4};
    } */

    std::size_t max_num_collisions_each_rank = static_cast<std::size_t>(5 * std::ceil(static_cast<double>(
        collision_manager->get_num_collisions()) / 5));
    std::size_t block_size = sizeof(Collision) * max_num_collisions_each_rank;

    shared_memory_manager = new SharedMemoryManager(rank, block_size);

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

    for (int i = 0; i < 1; ++i) {
        shmResponseWorkers.push_back(std::thread(handle_shared_memory_pending_responses, i, rank));
    }

    //int port = 50051 + rank; // 50051, 50052, 50053, etc.

    std::string server_addresss = myconfig->getIP() + ":" + std::to_string(myconfig->getPortNumber());
    service.Run(server_addresss);

    cleanup_worker_threads();
    cleanup_shared_memory();

    return 0;
}
