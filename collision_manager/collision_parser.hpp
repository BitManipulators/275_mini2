#pragma once

#include "collision.hpp"

#include <string>


class CollisionParser {

public:
    CollisionParser(const std::string& filename);
    Collisions parse();
    Collisions parsePartition(int start_index, int end_index);
    int getTotalRecords();

private:
    std::string filename;
};
