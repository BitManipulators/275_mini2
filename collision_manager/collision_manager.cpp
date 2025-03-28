#include "collision_manager.hpp"

#include "collision_parser.hpp"
#include "query.hpp"
#include <fstream>
#include <cstring>
#include <string>

#include <omp.h>

CollisionManager::CollisionManager(const std::string& filename) {
    CollisionParser parser{filename};

    try {
        int totalRecords = parser.getTotalRecords();

        int totalPartitions = 5;
        if (const char *env_total = std::getenv("TOTAL_PARTITIONS"))
        {
            totalPartitions = std::atoi(env_total);
        }

        int rank = 0;
        if (const char *env_rank = std::getenv("RANK"))
        {
            rank = std::atoi(env_rank);
        }

        int partitionSize = totalRecords / totalPartitions;
        int start_index = rank * partitionSize;
        int end_index = (rank == totalPartitions - 1) ? totalRecords : (start_index + partitionSize);

        std::cout << "Process with rank " << rank
                  << " will process records from " << start_index
                  << " to " << end_index - 1 << std::endl;

       // Collisions collisions = parser.parse();
       Collisions collisions = parser.parsePartition(start_index, end_index);

        this->indexed_collisions_ = IndexedCollisions(collisions);
        this->initialization_error_ = "";
    } catch (const std::runtime_error& e) {
        this->initialization_error_ = e.what();
    }
}

CollisionManager::CollisionManager(Collisions& collisions) {
    this->indexed_collisions_ = IndexedCollisions(collisions);
}

CollisionManager::CollisionManager(const std::vector<Collision>& collisions_list) {
    Collisions collisions{};
    for (const Collision& collision : collisions_list) {
        collisions.add(collision);
    }
    this->indexed_collisions_ = IndexedCollisions(collisions);
}

bool CollisionManager::is_initialized() {
    return this->initialization_error_.empty();
}

const std::string& CollisionManager::get_initialization_error() {
    return this->initialization_error_;
}

const std::vector<Collision> CollisionManager::search(const Query& query) {
    const std::vector<CollisionProxy*> collision_proxy_results = searchOpenMp(query);

    std::vector<Collision> collision_results{};
    for (CollisionProxy* proxy : collision_proxy_results) {
        collision_results.push_back(collision_proxy_to_collision(*proxy));
    }
    return collision_results;
}

const std::vector<CollisionProxy*> CollisionManager::searchOpenMp(const Query& query) {
    const std::vector<FieldQuery>& field_queries = query.get();
    std::vector<CollisionProxy*> results;

    unsigned long num_threads = omp_get_max_threads();
    std::vector<std::vector<CollisionProxy*>> thread_local_results(num_threads);

    // Initialize all matches to true initialially
    std::uint8_t* matches_data = new std::uint8_t[indexed_collisions_.collisions_.size()];
    std::span matches = {matches_data, indexed_collisions_.collisions_.size()};
    memset(matches.data(), 1, matches.size());

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();

        int chunk_size = indexed_collisions_.collisions_.size() / num_threads;
        int start_index = thread_id * chunk_size;
        int end_index = (thread_id == num_threads - 1) ? indexed_collisions_.collisions_.size() : start_index + chunk_size;

        for (const FieldQuery& field_query : field_queries) {
            indexed_collisions_.match(field_query, start_index, end_index, matches);
        }

        #pragma omp barrier

        for (std::size_t index = 0; index < end_index - start_index; ++index) {
            if (matches[index + start_index]) {
                thread_local_results[thread_id].push_back(&indexed_collisions_.proxies_.at(start_index + index));
            }
        }
    }

    for (const auto& local_results : thread_local_results) {
        results.insert(results.end(), local_results.begin(), local_results.end());
    }

    delete[] matches_data;

    return results;
}
