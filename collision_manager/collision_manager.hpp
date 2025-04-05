#pragma once

#include "collision.hpp"
#include "query.hpp"

#include <string>


class CollisionManager {

public:
    CollisionManager(const std::string& filename);

    bool is_initialized();
    const std::string& get_initialization_error();
    const std::size_t get_num_collisions();
    const std::vector<Collision> search(const Query& query);
    const std::vector<CollisionProxy*> searchOpenMp(const Query& query);

    friend class CollisionManagerTest;

private:
    CollisionManager(Collisions& collisions);
    CollisionManager(const std::vector<Collision>& collisions);

    std::string initialization_error_;
    IndexedCollisions indexed_collisions_;
};
