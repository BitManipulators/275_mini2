#include <grpcpp/grpcpp.h>
#include <iostream>


#include "collision.grpc.pb.h"


void RunClient(){

    std::string target_str = "localhost:50051";
    
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(500 * 1024 * 1024);
    channel_args.SetMaxSendMessageSize(500 * 1024 * 1024);
    std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel (target_str, grpc::InsecureChannelCredentials(),channel_args);
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    collision_proto::QueryRequest request;


    /*collision_proto::QueryCondition* condition  = request.add_queries();
    condition->set_field(collision_proto::QueryFields::BOROUGH);
    condition->set_condition(collision_proto::Condition::EQUALS);
    condition->set_str_value("BROOKLYN"); */

    /* collision_proto::QueryCondition* condition1  = request.add_queries();
    condition1->set_field(collision_proto::QueryFields::ZIP_CODE);
    condition1->set_condition(collision_proto::Condition::EQUALS);
    condition1->set_int_value(11208); */



    collision_proto::QueryCondition* condition2  = request.add_queries();
    condition2->set_field(collision_proto::QueryFields::LATITUDE);
    condition2->set_condition(collision_proto::Condition::EQUALS);
    condition2->set_float_value(40.783268);

    collision_proto::QueryResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub->GetCollisions(&context, request, &response);

    if (status.ok()){

        std::cout << "Collision size "<< response.collision_size() << std::endl;

        for (const collision_proto::Collision& collision  : response.collision()){

            std::cout << "Name : "<< collision.borough() << " Zip_code : " << collision.zip_code() << collision.latitude() << std::endl;
        }

    } else{
        std::cout << "Error";
    }


}

int main(){
    RunClient();
    return 0;
}