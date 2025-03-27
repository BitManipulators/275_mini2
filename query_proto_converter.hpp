#pragma once

#include "collision_manager/collision.hpp"
#include "collision_manager/query.hpp"

#include <collision.grpc.pb.h>
#include <grpcpp/grpcpp.h>

struct QueryRequest {
    std::size_t id;
    std::vector<std::uint32_t> requested_by;
    Query query;
};


class QueryProtoConverter {
public:
    static QueryRequest deserialize(const collision_proto::QueryRequest& proto_query_request);
    static collision_proto::QueryRequest serialize(const QueryRequest& query_request);
};
