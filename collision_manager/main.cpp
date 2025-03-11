#include "collision.hpp"
#include "collision_manager.hpp"
#include "query.hpp"

#include <filesystem>
#include <iostream>
#include <string>


int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path to CSV file to be parsed> " << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    std::filesystem::path filepath(filename);
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "Provided filename " << filename << " does not exist." << std::endl;
        return 1;
    } else if (!std::filesystem::is_regular_file(filepath)) {
        std::cerr << "Provided filename " << filename << " is not a file." << std::endl;
        return 1;
    }

    CollisionManager collision_manager{filename};
    if (!collision_manager.is_initialized()) {
        std::cerr << "Failed to initialize collisions: " << collision_manager.get_initialization_error() << std::endl;
        return 1;
    }

/*
    std::vector<const Collision*> collisions = collision_manager.search();
    std::cout << *collisions.at(0) << std::endl;
    std::cout << *collisions.at(1) << std::endl;
    std::cout << *collisions.at(2) << std::endl;
    std::cout << *collisions.at(3) << std::endl;
    std::cout << *collisions.at(4) << std::endl;
    std::cout << *collisions.at(5) << std::endl;
*/
    Query query = Query::create(CollisionField::LATITUDE, QueryType::LESS_THAN, 100000.0f);
    Query query2 = Query::create(CollisionField::COLLISION_ID, QueryType::LESS_THAN, 10000ULL);
    Query query3 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
    //Query query = Query::create("crash_date", QueryType::LESS_THAN, 1ULL);
    //Query query = Query::create("crash_time", QueryType::LESS_THAN, 1ULL);

    std::vector<CollisionProxy*> collisions = collision_manager.searchOpenMp(query3);
    std::cout << "Number collisions found: " << collisions.size() << std::endl;
    std::cout << *collisions.at(0) << std::endl;
    std::cout << *collisions.at(1) << std::endl;
    std::cout << *collisions.at(2) << std::endl;
    std::cout << *collisions.at(3) << std::endl;
    std::cout << *collisions.at(4) << std::endl;


    Query query4 = Query::create(CollisionField::BOROUGH, Qualifier::NOT, QueryType::EQUALS, "BROOKLYN");
    collisions = collision_manager.searchOpenMp(query4);
    std::cout << "Number collisions found: " << collisions.size() << std::endl;
    std::cout << *collisions.at(0) << std::endl;
    std::cout << *collisions.at(1) << std::endl;
    std::cout << *collisions.at(2) << std::endl;
    std::cout << *collisions.at(3) << std::endl;
    std::cout << *collisions.at(4) << std::endl;
}
