#include <grpcpp/grpcpp.h>
#include <iostream>


#include "collision.grpc.pb.h"


void RunClient(){

    std::string target_str = "localhost:50051";
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel (target_str, grpc::InsecureChannelCredentials());
    std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub = collision_proto::CollisionQueryService::NewStub(channel);

    collision_proto::QueryRequest request;
   
    
    collision_proto::QueryCondition* condition  = request.add_queries();
    condition->set_field(collision_proto::QueryFields::BOROUGH);
    condition->set_condition(collision_proto::Condition::EQUALS);
    condition->set_str_data("QUEENS");

    collision_proto::QueryCondition* condition1  = request.add_queries();
    condition1->set_field(collision_proto::QueryFields::ZIP_CODE);
    condition1->set_condition(collision_proto::Condition::EQUALS);
    condition1->set_int_data(11413); 

    collision_proto::QueryResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub->GetCollisions(&context, request, &response);

    if (status.ok()){
        
        std::cout << "Collision size "<< response.collision_size() << std::endl;

        for (const collision_proto::Collision& collision  : response.collision()){

            std::cout << "Name : "<< collision.borough() << "Date :" << collision.zip_code() << std::endl; 
        }
        
        

    }else{
        std::cout << "Error";
    }


}

int main(){
    RunClient();
    return 0;
}