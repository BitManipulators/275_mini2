#include "collision_query_service_impl.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>

std::atomic<std::size_t> requestCounter = 1;
std::size_t latestQueryID = 0;

CollisionQueryServiceImpl::CollisionQueryServiceImpl(
    std::uint32_t rank,
    std::mutex& pending_requests_mutex,
    std::mutex& pending_responses_mutex,
    PendingRequestsRingbuffer& pending_requests,
    PendingResponsesRingbuffer& pending_responses,
    std::condition_variable& pending_requests_cv,
    std::condition_variable& pending_responses_cv,
    std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map)
    : rank_{rank}
    , pending_requests_mutex_{pending_requests_mutex}
    , pending_responses_mutex_{pending_responses_mutex}
    , pending_requests_{pending_requests}
    , pending_responses_{pending_responses}
    , pending_requests_cv_{pending_requests_cv}
    , pending_responses_cv_{pending_responses_cv}
    , pending_client_requests_map_{pending_client_requests_map} {}

CollisionQueryServiceImpl::~CollisionQueryServiceImpl() {
    server_->Shutdown();
    cq_->Shutdown();
}

void CollisionQueryServiceImpl::Run(const int port) {
    std::string server_address = "127.0.0.1:" + std::to_string(port);
//    std::string server_address = "0.0.0.0:" + std::to_string(port);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();

    std::cout << "Server listening on " << server_address << std::endl;

    HandleRpcs();
}

void CollisionQueryServiceImpl::HandleRpcs() {
    new SendRequestCallData(this,
                            cq_.get(),
                            pending_requests_mutex_,
                            pending_requests_,
                            pending_requests_cv_);
    new ReceiveResponseCallData(this,
                                cq_.get(),
                                pending_responses_mutex_,
                                pending_responses_,
                                pending_responses_cv_);

    if (rank_ == 0) {
        new GetCollisionsCallData(this,
                                  cq_.get(),
                                  rank_,
                                  pending_requests_mutex_,
                                  pending_requests_,
                                  pending_requests_cv_,
                                  pending_client_requests_map_);
    }

    void* tag;
    bool ok;

    while (cq_->Next(&tag, &ok)) {
        static_cast<CallDataBase*>(tag)->Proceed(ok);
    }
}


GetCollisionsCallData::GetCollisionsCallData(
    CollisionQueryServiceImpl* service,
    ServerCompletionQueue* cq,
    std::uint32_t rank,
    std::mutex& pending_requests_mutex,
    PendingRequestsRingbuffer& pending_requests,
    std::condition_variable& pending_requests_cv,
    std::unordered_map<std::size_t, GetCollisionsClientRequest>& pending_client_requests_map)
    : service_(service)
    , cq_(cq)
    , responder_(&ctx_)
    , status_(CREATE)
    , rank_(rank)
    , pending_requests_mutex_(pending_requests_mutex)
    , pending_requests_(pending_requests)
    , pending_requests_cv_(pending_requests_cv)
    , pending_client_requests_map_(pending_client_requests_map)
{
    Proceed(true);
}

void GetCollisionsCallData::Proceed(bool ok) {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestGetCollisions(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
        new GetCollisionsCallData(service_, cq_, rank_, pending_requests_mutex_, pending_requests_, pending_requests_cv_, pending_client_requests_map_);

        QueryRequest query_request = QueryProtoConverter::deserialize(request_);

        {
            std::lock_guard<std::mutex> lock(pending_requests_mutex_);

            query_request.id = requestCounter.fetch_add(1);

            GetCollisionsClientRequest client_request {
                .collisions = {},
                .ranks = {},
                .call_data_base = this,
            };

            auto [it, inserted] = pending_client_requests_map_.try_emplace(query_request.id, client_request);
            if (!inserted) {
                std::cout << "Request with id: '" << query_request.id << "' already in map!" << std::endl;
                status_ = FINISH;

                QueryResponse query_response {
                   .id = query_request.id,
                   .requested_by = {},
                   .results_from = rank_,
                   .collisions = {},
                };

                collision_proto::QueryResponse response = CollisionProtoConverter::serialize(query_response);

                responder_.Finish(response, Status::CANCELLED, this);
                return;
            }

            pending_requests_.push(query_request);
            pending_requests_cv_.notify_one();
        }

        std::cout << "Added request from: 'client' with id: '" << query_request.id <<
                     "' to the pendingRequests queue" << std::endl;
    } else {
        assert(status_ == FINISH);
        delete this;
    }
}

void GetCollisionsCallData::CompleteRequest(const std::size_t id,
                                            const std::vector<Collision>& collisions) {
    QueryResponse query_response {
        .id = id,
        .requested_by = {},
        .results_from = rank_,
        .collisions = collisions,
    };

    collision_proto::QueryResponse response = CollisionProtoConverter::serialize(query_response);

    status_ = FINISH;
    responder_.Finish(response, Status::OK, this);
}


SendRequestCallData::SendRequestCallData(
    CollisionQueryServiceImpl* service,
    ServerCompletionQueue* cq,
    std::mutex& pending_requests_mutex,
    PendingRequestsRingbuffer& pending_requests,
    std::condition_variable& pending_requests_cv)
    : service_(service)
    , cq_(cq)
    , responder_(&ctx_)
    , status_(CREATE)
    , pending_requests_mutex_(pending_requests_mutex)
    , pending_requests_(pending_requests)
    , pending_requests_cv_(pending_requests_cv)
{
    Proceed(true);
}

void SendRequestCallData::Proceed(bool ok) {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestSendRequest(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
        new SendRequestCallData(service_, cq_, pending_requests_mutex_, pending_requests_, pending_requests_cv_);

        QueryRequest query_request = QueryProtoConverter::deserialize(request_);

        {
            std::lock_guard<std::mutex> lock(pending_requests_mutex_);
            if (latestQueryID < query_request.id) {
                latestQueryID = query_request.id;
                pending_requests_.push(query_request);
                pending_requests_cv_.notify_one();

                std::cout << "Added request from: '" << static_cast<char>('A' + query_request.requested_by.back()) <<
                             "' with id: '" << query_request.id << "' to the pendingRequests queue" << std::endl;
            } else {
                std::cout << "Dropping duplicate query with id: " << query_request.id << std::endl;
            }
        }

        Empty response;
        status_ = FINISH;
        responder_.Finish(response, Status::OK, this);
    } else {
        assert(status_ == FINISH);
        delete this;
    }
}


ReceiveResponseCallData::ReceiveResponseCallData(
    CollisionQueryServiceImpl* service,
    ServerCompletionQueue* cq,
    std::mutex& pending_responses_mutex,
    PendingResponsesRingbuffer& pending_responses,
    std::condition_variable& pending_responses_cv)
    : service_(service)
    , cq_(cq)
    , responder_(&ctx_)
    , status_(CREATE)
    , pending_responses_mutex_(pending_responses_mutex)
    , pending_responses_(pending_responses)
    , pending_responses_cv_(pending_responses_cv)
{
    Proceed(true);
}

void ReceiveResponseCallData::Proceed(bool ok) {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestReceiveResponse(&ctx_, &response_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
        new ReceiveResponseCallData(service_, cq_, pending_responses_mutex_, pending_responses_, pending_responses_cv_);

        QueryResponse query_response = CollisionProtoConverter::deserialize(response_);

        std::cout << "Received response from: '" << static_cast<char>('A' + query_response.requested_by.back()) <<
                     "' with id: '" << response_.id() <<
                     "' originally from: '" << static_cast<char>('A' + query_response.results_from) << "'"
                  << std::endl;

        query_response.requested_by.pop_back();

        {
            std::lock_guard<std::mutex> lock(pending_responses_mutex_);
            pending_responses_.push(query_response);
            pending_responses_cv_.notify_one();
        }

        Empty response;
        status_ = FINISH;
        responder_.Finish(response, Status::OK, this);
    } else {
        assert(status_ == FINISH);
        delete this;
    }
}
