#include "collision_manager/collision_manager.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "myconfig.hpp"
#include <omp.h>
#include <grpcpp/grpcpp.h>
#include <future>
#include <filesystem>
#include <iostream>
#include <atomic>
#include <map>
#include <vector>


const std::string CSV_FILE = std::string("/home/suriya-018231499/cpp_projects/parser_data/Motor_Vehicle_Collisions_-_Crashes_20250204.csv");

static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(CSV_FILE);

std::mutex queryMutex;
std::unordered_set<int> processedQueries;
std::atomic<int> counter(0);


QueryResponse queryPeer(const std::string peer_address, const collision_proto::QueryRequest &request)
{
    
    std::cout << "Requesting the neighbour : "<< peer_address << std::endl ;
    std::vector<collision_proto::Collision> peerCollisions;
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials());
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    collision_proto::QueryResponse peerResponse;
    grpc::ClientContext clientContext;

    // Set a deadline (e.g., 2 seconds from now)
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
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
        
        CollisionQueryServiceImpl(int r, const std::vector<std::string>& peers) : rank(r), peer_addresses_(peers) {}
        
        
        grpc::Status GetCollisions(grpc::ServerContext* context,
                                const collision_proto::QueryRequest* request,
                                collision_proto::QueryResponse* response) override {

            
            
            QueryRequest query_request = QueryProtoConverter::deserialize(*request);

            if (rank == 0){
                query_request.id = counter.fetch_add(1);
            }
            
            //Check for duplicates    
            
            if (rank != 0){

                int request_id = request->id();
            
                std::lock_guard<std::mutex> lock(queryMutex);
                if (processedQueries.find(request_id) != processedQueries.end()) {
                    std::cerr << "Duplicate query detected in Process " << rank  << " Request Id - " << request_id << " INFO :: Rejecting query " << std::endl;
                    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Query already processed");
                }
                // Mark query as processed.
                processedQueries.insert(request_id);
            }
            
            
            // Fetch On server Results
            std::vector<Collision> localResults = collision_manager->search(query_request.query);

            std::cout <<"Process - Rank " << rank << " Local collision size: " << localResults.size() << std::endl;

            std::vector<Collision> aggregatedResults{};
            aggregatedResults.insert(aggregatedResults.end(), localResults.begin(), localResults.end());

            
            collision_proto::QueryRequest forward_request = QueryProtoConverter::serialize(query_request);
            for (const auto& neighbour : peer_addresses_){
                
                QueryResponse results = queryPeer(neighbour, forward_request);
                aggregatedResults.insert(aggregatedResults.end(), results.collisions.begin(), results.collisions.end());
                std::cout << "Results from  " <<  neighbour << " Collision size "<<results.collisions.size() << std::endl;
            }
            
            
            
            std::cout << "Process - Rank " << rank << " Aggregated collision size: " << aggregatedResults.size() << std::endl;
            

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

    
    private :
        std::vector<std::string> peer_addresses_;   
        int rank; 


};


void RunServer() {

    
    
    MyConfig*  myconfig = MyConfig::getInstance();
    int port = myconfig->getPortNumber();
    
    std::cout << "My Info Rank - " << myconfig->getRank() << " PORT - " << myconfig->getPortNumber() << std::endl ;
    
    std::string myaddress = "127.0.0.1:" + std::to_string(port);
    
    for (const auto& n : myconfig->getLogicalNeighbors()){
        std::cout << "Neighbor - " << n << std::endl;
    }

    CollisionQueryServiceImpl service(myconfig->getRank(), myconfig->getLogicalNeighbors());

    grpc::ServerBuilder builder;
    builder.AddListeningPort(myaddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server Listening on " << myaddress << std::endl;

    server->Wait();
}

int main(int argc, char *argv[]) {
    
    RunServer();
    return 0;
}
