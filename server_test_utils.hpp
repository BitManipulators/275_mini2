#pragma once

#include "collision_proto_converter.hpp"
#include "query_proto_converter.hpp"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

// Common test utilities for all server tests
class ServerTestUtils {
public:
    // Timeout for gRPC calls (in seconds)
    static inline const int RPC_TIMEOUT = 10;

    // Helper function to create a test query that should return data from all nodes
    static Query createTestQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::CONTAINS, "");
    }

    // Helper function to create a query specific to Brooklyn with ZIP 11233
    static Query createBrooklynQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
            .add(CollisionField::ZIP_CODE, QueryType::EQUALS, static_cast<uint32_t>(11233));
    }

    // Helper function to create a query for Manhattan
    static Query createManhattanQuery() {
        return Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "MANHATTAN");
    }

    // Helper function to query a server
    static std::vector<Collision> queryServer(int base_port, const Query& query) {
        // We always query process A (rank 0), which is at base_port
        std::string target_str = "127.0.0.1:" + std::to_string(base_port);
        grpc::ChannelArguments channel_args;
        channel_args.SetMaxReceiveMessageSize(13550247);
        std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(
            target_str, grpc::InsecureChannelCredentials(), channel_args);
        std::unique_ptr<collision_proto::CollisionQueryService::Stub> stub =
            collision_proto::CollisionQueryService::NewStub(channel);

        QueryRequest query_request = {
            .id = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()),
            .requested_by = {std::numeric_limits<std::uint32_t>::max()}, // Client ID
            .query = query,
        };

        collision_proto::QueryRequest request = QueryProtoConverter::serialize(query_request);

        collision_proto::QueryResponse response;
        grpc::ClientContext context;

        // Set timeout
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(RPC_TIMEOUT);
        context.set_deadline(deadline);

        std::cout << "Sending query to server at port " << base_port << std::endl;
        grpc::Status status = stub->GetCollisions(&context, request, &response);

        if (status.ok()) {
            auto query_response = CollisionProtoConverter::deserialize(response);
            std::cout << "Received " << query_response.collisions.size()
                      << " collisions from server at port " << base_port << std::endl;
            return query_response.collisions;
        } else {
            std::cerr << "RPC Error on port " << base_port << ": "
                      << status.error_code() << ": " << status.error_message()
                      << " (" << status.error_details() << ")" << std::endl;
            return {};
        }
    }

    // Helper to log statistics about the results
    static void logResults(const std::string& server_type, const std::vector<Collision>& collisions) {
        std::cout << "==== " << server_type << " Results ====" << std::endl;
        std::cout << "Total collisions: " << collisions.size() << std::endl;

        // Count by borough
        std::unordered_map<std::string, int> borough_counts;
        std::unordered_map<uint32_t, int> zip_counts;

        for (const auto& collision : collisions) {
            if (collision.borough.has_value()) {
                // Use the FixedString directly as a C-string
                const auto& borough_val = collision.borough.value();
                // Convert to std::string for use as map key
                std::string borough_str(borough_val.c_str());
                borough_counts[borough_str]++;
            } else {
                borough_counts["UNKNOWN"]++;
            }

            if (collision.zip_code.has_value()) {
                zip_counts[collision.zip_code.value()]++;
            }
        }

        std::cout << "Borough distribution:" << std::endl;
        for (const auto& [borough, count] : borough_counts) {
            std::cout << "  " << borough << ": " << count << std::endl;
        }

        // Print top 5 zip codes
        std::vector<std::pair<uint32_t, int>> zip_vec(zip_counts.begin(), zip_counts.end());
        std::sort(zip_vec.begin(), zip_vec.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        std::cout << "Top 5 ZIP codes:" << std::endl;
        for (size_t i = 0; i < std::min(5UL, zip_vec.size()); i++) {
            std::cout << "  " << zip_vec[i].first << ": " << zip_vec[i].second << std::endl;
        }

        // Print first few records for manual verification
        const int MAX_RECORDS_TO_PRINT = 5;
        std::cout << "First " << std::min(MAX_RECORDS_TO_PRINT, static_cast<int>(collisions.size()))
                 << " records:" << std::endl;

        for (size_t i = 0; i < std::min(static_cast<size_t>(MAX_RECORDS_TO_PRINT), collisions.size()); i++) {
            const auto& collision = collisions[i];
            std::cout << "  " << i + 1 << ". ID: ";

            if (collision.collision_id.has_value()) {
                std::cout << collision.collision_id.value();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << ", Borough: ";
            if (collision.borough.has_value()) {
                // Use c_str() to access the character data
                std::cout << collision.borough.value().c_str();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << ", ZIP: ";
            if (collision.zip_code.has_value()) {
                std::cout << collision.zip_code.value();
            } else {
                std::cout << "UNKNOWN";
            }

            std::cout << std::endl;
        }

        std::cout << std::endl;
    }

    // Verify that a set of collisions match a specific borough and ZIP code
    static bool verifyBrooklynCollisions(const std::vector<Collision>& collisions) {
        for (const auto& collision : collisions) {
            bool is_brooklyn = collision.borough.has_value() &&
                               std::string(collision.borough.value().c_str()) == "BROOKLYN";

            bool has_zip = collision.zip_code.has_value() &&
                          collision.zip_code.value() == 11233;

            if (!is_brooklyn || !has_zip) {
                std::cout << "Invalid record found - Borough: ";
                if (collision.borough.has_value()) {
                    std::cout << collision.borough.value().c_str();
                } else {
                    std::cout << "UNKNOWN";
                }

                std::cout << ", ZIP: ";
                if (collision.zip_code.has_value()) {
                    std::cout << collision.zip_code.value();
                } else {
                    std::cout << "UNKNOWN";
                }

                std::cout << std::endl;
                return false;
            }
        }
        return true;
    }

    // Verify that a set of collisions match Manhattan
        static bool verifyManhattanCollisions(const std::vector<Collision>& collisions) {
        for (const auto& collision : collisions) {
            bool is_manhattan = collision.borough.has_value() &&
                               std::string(collision.borough.value().c_str()) == "MANHATTAN";

            // bool has_zip = collision.zip_code.has_value() &&
            //               collision.zip_code.value() == 10017;

            if (!is_manhattan) {
                std::cout << "Invalid record found - Borough: ";
                if (collision.borough.has_value()) {
                    std::cout << collision.borough.value().c_str();
                } else {
                    std::cout << "UNKNOWN";
                }

                // std::cout << ", ZIP: ";
                // if (collision.zip_code.has_value()) {
                //     std::cout << collision.zip_code.value();
                // } else {
                //     std::cout << "UNKNOWN";
                // }

                std::cout << std::endl;
                return false;
            }
        }
        return true;
    }

};
