#pragma once

#include "collision_manager/collision.hpp"
#include "collision_manager/collision_parser.hpp"

#include <collision.grpc.pb.h>
#include <grpcpp/grpcpp.h>

struct QueryResponse {
    std::size_t id;
    std::vector<std::uint32_t> requested_by;
    std::uint32_t results_from;
    std::vector<Collision> collisions;
};

class CollisionProtoConverter {
public:
    static QueryResponse deserialize(const collision_proto::QueryResponse& proto_query_response);
    static collision_proto::QueryResponse serialize(const QueryResponse& query_response);
    static void serialize(const QueryResponse& query_response, collision_proto::QueryResponse& proto_query_response);
};
