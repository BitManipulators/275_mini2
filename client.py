#!/usr/bin/env python3
import grpc
import collision_pb2
import collision_pb2_grpc

def run():
    channel = grpc.insecure_channel('localhost:50051')
    stub = collision_pb2_grpc.CollisionQueryServiceStub(channel)
    request = collision_pb2.QueryRequest()

    condition = request.queries.add()
    condition.field = collision_pb2.QueryFields.BOROUGH
    condition.condition = collision_pb2.Condition.EQUALS
    condition.str_data = "BROOKLYN"

    condition1 = request.queries.add()
    condition1.field = collision_pb2.QueryFields.ZIP_CODE
    condition1.condition = collision_pb2.Condition.EQUALS
    condition1.int_data = 11208

    try:
        response = stub.GetCollisions(request)
        print("Collision size:", len(response.collision))

        for collision in response.collision:
            print("Name:", collision.borough, "Zip_code:", collision.zip_code)

    except grpc.RpcError as e:
        print(f"RPC failed: {e}")

if __name__ == "__main__":
    run()