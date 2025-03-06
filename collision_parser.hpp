#pragma once

#include "collision.hpp"

#include <string>


class CollisionParser {

public:
    CollisionParser(const std::string& filename);
    Collisions parse();

private:
    std::string filename;
};
