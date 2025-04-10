#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"

#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "myconfig.hpp"

using google::protobuf::Empty;
int MASTER = 0;
void RunClient() {

    Config config;
    // Set Master process IP
    std::string target_str = config.getIP(MASTER) + ":" + std::to_string(config.getPortNumber(MASTER));
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    Query query = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
        .add(CollisionField::ZIP_CODE, QueryType::EQUALS, static_cast<uint32_t>(11233));

    QueryRequest query_request = {
        .id = 1,
        .requested_by = {std::numeric_limits<std::uint32_t>::max()},
        .query = query,
    };

    collision_proto::QueryRequest request = QueryProtoConverter::serialize(query_request);

    collision_proto::QueryResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub->GetCollisions(&context, request, &response);

    std::cout << "Sent query" << std::endl;

    if (status.ok()) {

        std::cout << "Collision size "<< response.collision_size() << std::endl;

        //for (const collision_proto::Collision& collision  : response.collision()) {

            //std::cout << "Name : " << collision.borough() << " Zip_code : " << collision.zip_code() << std::endl;
        //}

    } else {
    std::cerr << "RPC Error: " << status.error_code() << ": " << status.error_message()
              << " (" << status.error_details() << ")" << std::endl;
    }
}

int main(){
    
    RunClient();
    
    /* auto start = std::chrono::high_resolution_clock::now();
    
    const int numThreads = 5;
    std::thread threads[numThreads];

    for (int i = 0; i < numThreads ; i++){
        threads[i] = std::thread(RunClient);
    }

    for (int i = 0; i < numThreads ; i++){
        threads[i].join();
    }

    //std::thread t(RunClient);
    //t.join();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start ;

    std::cout << "Elasped Time : " << duration.count() << "seconds" << std::endl; */

    return 0;
}
