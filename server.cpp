#include "collision_manager/collision_manager.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"

#include <grpcpp/grpcpp.h>

#include <filesystem>
#include <iostream>


const std::string CSV_FILE = std::string("../MotorVehicleCollisionData_subset.csv");

static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(CSV_FILE);

class CollisionQueryServiceImpl final : public collision_proto::CollisionQueryService::Service {
public:
    grpc::Status GetCollisions(grpc::ServerContext* context,
                               const collision_proto::QueryRequest* request,
                               collision_proto::QueryResponse* response) override {

        Query query = QueryProtoConverter::deserialize(request);
        std::vector<CollisionProxy*> collisions = collision_manager->searchOpenMp(query);

        std::cout << "collison size" << collisions.size() << std::endl;

        int counter= 100;
        for (auto proxy : collisions ){
            if (counter == 0){
                break;
            }
            collision_proto::Collision* collision = response->add_collision();
            CollisionProtoConverter::serialize(proxy, collision);
            counter--;
        }

        return grpc::Status::OK;
    }
};

void RunServer() {

    std::string server_address("0.0.0.0:50051");
    CollisionQueryServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server Listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char *argv[]) {

    RunServer();
    return 0;
}
