#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"

#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "myconfig.hpp"

using google::protobuf::Empty;
int MASTER = 0;

void RunClient(bool stream) {

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

    std::cout << "Sending query" << std::endl;

    std::vector<Collision> collisions{};

    grpc::Status status;
    if (stream) {
        std::unique_ptr<grpc::ClientReader<collision_proto::QueryResponse>> reader(stub->StreamCollisions(&context, request));

        std::size_t total_collision_size = 0;
        std::size_t num_responses = 0;
        while (reader->Read(&response)) {
            QueryResponse query_response = CollisionProtoConverter::deserialize(response);
            collisions.insert(collisions.end(), query_response.collisions.begin(), query_response.collisions.end());

            std::cout << "Received streaming response number: " << num_responses << std::endl;
            ++num_responses;
        }

        status = reader->Finish();
    } else {
        status = stub->GetCollisions(&context, request, &response);

        QueryResponse query_response = CollisionProtoConverter::deserialize(response);
        collisions.insert(collisions.end(), query_response.collisions.begin(), query_response.collisions.end());
    }

    if (status.ok()) {
        std::cout << "Collision size "<< collisions.size() << std::endl;
/*
        for (const Collision& collision  : collisions) {
            std::cout << "Name : " << collision.borough.value().c_str() << " Zip_code : " << collision.zip_code.value() << std::endl;
        }
*/
    } else {
        std::cerr << "RPC Error: " << status.error_code() << ": " << status.error_message()
                  << " (" << status.error_details() << ")" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    bool stream = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stream") {
            stream = true;
            break; // Found it, no need to continue
        }
    }

    RunClient(stream);
    return 0;
}
