#pragma once

#include "collision_manager/collision.hpp"
#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include "ring_buffer.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <collision.grpc.pb.h>
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::CompletionQueue;
using grpc::ServerCompletionQueue;
using grpc::Status;

using google::protobuf::Empty;

constexpr std::size_t MAX_CONCURRENT_REQUESTS = 100;
using PendingRequestsRingbuffer = Ringbuffer<QueryRequest, MAX_CONCURRENT_REQUESTS>;

constexpr std::size_t MAX_CONCURRENT_RESPONSES = 5 * MAX_CONCURRENT_REQUESTS;
using PendingResponsesRingbuffer = Ringbuffer<QueryResponse, MAX_CONCURRENT_RESPONSES>;

class CallDataBase {
public:
    virtual ~CallDataBase() = default;
    virtual void Proceed(bool ok) = 0;
};


struct GetCollisionsClientRequest {
    std::vector<Collision> collisions;
    std::vector<uint32_t> ranks;
    CallDataBase* call_data_base;
};

struct StreamCollisionsClientRequest {
    std::mutex write_mutex;
    std::vector<uint32_t> ranks;
    CallDataBase* call_data_base;

    StreamCollisionsClientRequest(CallDataBase* call_data_base_ptr)
      : call_data_base(call_data_base_ptr) {}
};

class CollisionQueryServiceImpl final : public collision_proto::CollisionQueryService::AsyncService {
public:
    CollisionQueryServiceImpl(
        std::uint32_t rank,
        std::mutex& pending_requests_mutex,
        std::mutex& pending_client_requests_mutex,
        std::mutex& pending_responses_mutex,
        PendingRequestsRingbuffer& pending_requests,
        PendingResponsesRingbuffer& pending_responses,
        std::condition_variable& pending_requests_cv,
        std::condition_variable& pending_responses_cv,
        std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map,
        std::unordered_map<std::size_t, StreamCollisionsClientRequest>& pending_stream_requests_map);
    ~CollisionQueryServiceImpl();

    void Run(const std::string server_address);

private:
    void HandleRpcs();

    std::uint32_t rank_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;

    std::mutex& pending_requests_mutex_;
    std::mutex& pending_client_requests_mutex_;
    std::mutex& pending_responses_mutex_;
    PendingRequestsRingbuffer& pending_requests_;
    PendingResponsesRingbuffer& pending_responses_;
    std::condition_variable& pending_requests_cv_;
    std::condition_variable& pending_responses_cv_;
    std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map_;
    std::unordered_map<std::size_t, StreamCollisionsClientRequest>& pending_stream_requests_map_;
};


class GetCollisionsCallData : public CallDataBase {
public:
    GetCollisionsCallData(CollisionQueryServiceImpl* service,
                          ServerCompletionQueue* cq,
                          std::uint32_t rank,
                          std::mutex& pending_requests_mutex,
                          std::mutex& pending_client_requests_mutex,
                          PendingRequestsRingbuffer& pending_requests,
                          std::condition_variable& pending_requests_cv,
                          std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map);

    void Proceed(bool ok) override;
    void CompleteRequest(const std::size_t id,
                         const std::vector<Collision>& collisions);

private:
    CollisionQueryServiceImpl* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    collision_proto::QueryRequest request_;
    ServerAsyncResponseWriter<collision_proto::QueryResponse> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    std::uint32_t rank_;
    std::mutex& pending_requests_mutex_;
    std::mutex& pending_client_requests_mutex_;
    PendingRequestsRingbuffer& pending_requests_;
    std::condition_variable& pending_requests_cv_;
    std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map_;
};


class StreamCollisionsCallData : public CallDataBase {
public:
    StreamCollisionsCallData(CollisionQueryServiceImpl* service,
                             ServerCompletionQueue* cq,
                             std::uint32_t rank,
                             std::mutex& pending_requests_mutex,
                             std::mutex& pending_client_requests_mutex,
                             PendingRequestsRingbuffer& pending_requests,
                             std::condition_variable& pending_requests_cv,
                             std::unordered_map<std::size_t, StreamCollisionsClientRequest>& pending_stream_requests_map);

    void Proceed(bool ok) override;
    void Write(const std::size_t id,
               const std::uint32_t results_from,
               const std::vector<Collision>& collisions,
               std::unique_lock<std::mutex>&& write_lock);
    void Finish(const std::size_t id);

private:
    CollisionQueryServiceImpl* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    collision_proto::QueryRequest request_;
    ServerAsyncWriter<collision_proto::QueryResponse> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    std::uint32_t rank_;
    std::mutex& pending_requests_mutex_;
    std::mutex& pending_client_requests_mutex_;
    PendingRequestsRingbuffer& pending_requests_;
    std::condition_variable& pending_requests_cv_;
    std::unordered_map<std::size_t, StreamCollisionsClientRequest>& pending_stream_requests_map_;
};


class StreamCollisionsWriteTagCallData : public CallDataBase {
public:
    StreamCollisionsWriteTagCallData(std::size_t id, std::uint32_t results_from, std::unique_lock<std::mutex>&& lock);

    void Proceed(bool ok) override;

private:
    std::size_t id_;
    std::uint32_t results_from_;
    std::unique_lock<std::mutex> lock_;
};


class SendRequestCallData : public CallDataBase {
public:
    SendRequestCallData(CollisionQueryServiceImpl* service,
                        ServerCompletionQueue* cq,
                        std::mutex& pending_requests_mutex,
                        PendingRequestsRingbuffer& pending_requests,
                        std::condition_variable& pending_requests_cv);

    void Proceed(bool ok) override;

private:
    CollisionQueryServiceImpl* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    collision_proto::QueryRequest request_;
    ServerAsyncResponseWriter<Empty> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    std::mutex& pending_requests_mutex_;
    PendingRequestsRingbuffer& pending_requests_;
    std::condition_variable& pending_requests_cv_;
};


class ReceiveResponseCallData : public CallDataBase {
public:
    ReceiveResponseCallData(CollisionQueryServiceImpl* service,
                            ServerCompletionQueue* cq,
                            std::mutex& pending_responses_mutex,
                            PendingResponsesRingbuffer& pending_responses,
                            std::condition_variable& pending_responses_cv);

    void Proceed(bool ok) override;

private:
    CollisionQueryServiceImpl* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    collision_proto::QueryResponse response_;
    ServerAsyncResponseWriter<Empty> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    std::mutex& pending_responses_mutex_;
    PendingResponsesRingbuffer& pending_responses_;
    std::condition_variable& pending_responses_cv_;
};
