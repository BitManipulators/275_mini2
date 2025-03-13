#pragma once

#include "collision_manager/collision.hpp"

#include <collision.grpc.pb.h>
#include <grpcpp/grpcpp.h>

class CollisionProtoConverter {
public:
    static Collision deserialize(const collision_proto::Collision* collision);
    static collision_proto::Collision serialize(const Collision& collision);
    static void serialize(const CollisionProxy* data_struct, collision_proto::Collision* proto_data);
};
