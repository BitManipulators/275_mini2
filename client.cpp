#include "query_proto_converter.hpp"

#include <grpcpp/grpcpp.h>
#include <iostream>


void RunClient() {

    std::string target_str = "localhost:50051";
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(500 * 1024 * 1024);
    //channel_args.SetMaxSendMessageSize(500 * 1024 * 1024);
    std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel (target_str, grpc::InsecureChannelCredentials(),channel_args);
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    Query query = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
        //.add(CollisionField::ZIP_CODE, QueryType::EQUALS, static_cast<uint32_t>(11208));

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
        std::cout << "Error";
    }
}

int main(){
    RunClient();
    return 0;
}
