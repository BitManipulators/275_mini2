#include "query_proto_converter.hpp"

#include <grpcpp/grpcpp.h>
#include <iostream>


void RunClient() {

    std::string target_str = "127.0.0.1:50051";
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    Query query = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
        .add(CollisionField::ZIP_CODE, QueryType::EQUALS, static_cast<uint32_t>(11233));

    collision_proto::QueryRequest request = QueryProtoConverter::serialize(query);

    collision_proto::QueryResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub->GetCollisions(&context, request, &response);

    if (status.ok()) {
        std::cout << "Collision size "<< response.collision_size() << std::endl;

        for (const collision_proto::Collision& collision  : response.collision()){

            std::cout << "Name : " << collision.borough() << " Zip_code : " << collision.zip_code() << std::endl;
        }
    } else {
    std::cerr << "RPC Error: " << status.error_code() << ": " << status.error_message()
              << " (" << status.error_details() << ")" << std::endl;
}
}

int main(){
    RunClient();
    return 0;
}
