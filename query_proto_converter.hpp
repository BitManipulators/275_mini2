#pragma once

#include "collision_manager/collision.hpp"
#include "collision_manager/query.hpp"

#include <collision.grpc.pb.h>
#include <grpcpp/grpcpp.h>

class QueryProtoConverter {
public:
    static Query deserialize(const collision_proto::QueryRequest* request);
    static collision_proto::QueryRequest serialize(const Query& query);
};
