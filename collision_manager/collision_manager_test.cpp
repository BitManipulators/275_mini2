#include "collision_manager.hpp"
#include <chrono>
#include <gtest/gtest.h>

namespace {
    const char* const kSubsetDataset = "../MotorVehicleCollisionData_subset.csv";
}

class CollisionManagerTest : public ::testing::Test {
protected:
    CollisionManagerTest() :
        collision_manager_m(CollisionManager("")) {}

    CollisionManager create_collision_manager(std::vector<Collision>& collisions) {
        return CollisionManager(collisions);
    }

    CollisionManager create_collision_manager_from_csv(const std::string& filename) {
        return CollisionManager(filename);
    }

    void SetUp(){
        if(!is_initialized_m) {
            std::string filename(kSubsetDataset);
            collision_manager_m = create_collision_manager_from_csv(filename);
            is_initialized_m = true;
        }
    }

    CollisionManager collision_manager_m;
    bool is_initialized_m = false;
};

TEST_F(CollisionManagerTest, ValidQueriesDoNotThrowExceptions) {
    // String field and parameter is a string
    EXPECT_NO_THROW({
        Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "Hello world");
    });

    // Size_t field and parameter is a size_t
    EXPECT_NO_THROW({
        Query::create(CollisionField::COLLISION_ID, QueryType::EQUALS, 100ULL);
    });

    // Date field and parameter is a date
    EXPECT_NO_THROW({
        Query::create(CollisionField::CRASH_DATE, QueryType::EQUALS, std::chrono::year_month_day{
            std::chrono::year{2021},
            std::chrono::month{9},
            std::chrono::day{11}}
        );
    });

    // Time field and parameter is a time
    EXPECT_NO_THROW({
        Query::create(CollisionField::CRASH_TIME, QueryType::EQUALS, std::chrono::hh_mm_ss<std::chrono::minutes>{
            std::chrono::hours{9} + std::chrono::minutes{35}}
        );
    });

    // Float field and parameter is a float
    EXPECT_NO_THROW({
        Query::create(CollisionField::LATITUDE, QueryType::EQUALS, 1.0f);
    });
}

TEST_F(CollisionManagerTest, InvalidQueriesThrowExceptions) {
    // String field but parameter is a float
    EXPECT_THROW({
        Query::create(CollisionField::BOROUGH, QueryType::EQUALS, 1.0f);
    }, std::invalid_argument);

    // Size_t field but parameter is a date
    EXPECT_THROW({
        Query::create(CollisionField::COLLISION_ID, QueryType::EQUALS, std::chrono::year_month_day{
            std::chrono::year{2021},
            std::chrono::month{9},
            std::chrono::day{11}}
        );
    }, std::invalid_argument);

    // Date field but parameter is a time
    EXPECT_THROW({
        Query::create(CollisionField::CRASH_DATE, QueryType::EQUALS, std::chrono::hh_mm_ss<std::chrono::minutes>{
            std::chrono::hours{9} + std::chrono::minutes{35}}
        );
    }, std::invalid_argument);

    // Time field but parameter is a string
    EXPECT_THROW({
        Query::create(CollisionField::CRASH_TIME, QueryType::EQUALS, "Hello world");
    }, std::invalid_argument);

    // Float field but parameter is a size_t
    EXPECT_THROW({
        Query::create(CollisionField::LATITUDE, QueryType::EQUALS, 100ULL);
    }, std::invalid_argument);
}

TEST_F(CollisionManagerTest, MatchEmpty) {
    std::vector<Collision> collisions{};
    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "Nothing should match me");

    std::vector<CollisionProxy*> results = collision_manager.searchOpenMp(query);
    EXPECT_EQ(results.size(), 0);
}

TEST_F(CollisionManagerTest, MatchEquals) {
    Collision collision1{};
    collision1.borough = "BROOKLYN";

    std::vector<Collision> collisions{collision1};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "Nothing should match me");
    std::vector<CollisionProxy*> results1 = collision_manager.searchOpenMp(query1);
    EXPECT_EQ(results1.size(), 0);

    Query query2 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
    std::vector<CollisionProxy*> results2 = collision_manager.searchOpenMp(query2);
    EXPECT_EQ(results2.size(), 1);
}

TEST_F(CollisionManagerTest, CompoundMatchEquals) {
    Collision collision1{};
    collision1.borough = "BROOKLYN";
    collision1.collision_id = 1;

    Collision collision2{};
    collision2.borough = "BROOKLYN";
    collision2.collision_id = 2;

    Collision collision3{};
    collision3.borough = "QUEENS";
    collision3.collision_id = 3;

    std::vector<Collision> collisions{collision1, collision2, collision3};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
        .add(CollisionField::COLLISION_ID, QueryType::EQUALS, 10ULL);
    std::vector<CollisionProxy*> results1 = collision_manager.searchOpenMp(query1);
    EXPECT_EQ(results1.size(), 0);

    Query query2 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN")
        .add(CollisionField::COLLISION_ID, QueryType::EQUALS, 1ULL);
    std::vector<CollisionProxy*> results2 = collision_manager.searchOpenMp(query2);
    EXPECT_EQ(results2.size(), 1);
    EXPECT_EQ(*results2[0]->borough, "BROOKLYN");
    EXPECT_EQ(*results2[0]->collision_id, 1ULL);

    Query query3 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "QUEENS")
        .add(CollisionField::COLLISION_ID, QueryType::EQUALS, 3ULL);
    std::vector<CollisionProxy*> results3 = collision_manager.searchOpenMp(query3);
    EXPECT_EQ(results3.size(), 1);
    EXPECT_EQ(*results3[0]->borough, "QUEENS");
    EXPECT_EQ(*results3[0]->collision_id, 3ULL);

    Query query4 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
    std::vector<CollisionProxy*> results4 = collision_manager.searchOpenMp(query4);
    EXPECT_EQ(results4.size(), 2);
    EXPECT_EQ(*results4[0]->borough, "BROOKLYN");
    EXPECT_EQ(*results4[0]->collision_id, 1ULL);
    EXPECT_EQ(*results4[1]->borough, "BROOKLYN");
    EXPECT_EQ(*results4[1]->collision_id, 2ULL);
}

TEST_F(CollisionManagerTest, MatchNotEquals) {
    Collision collision1{};
    collision1.borough = "BROOKLYN";

    Collision collision2{};
    collision2.borough = "QUEENS";

    std::vector<Collision> collisions{collision1, collision2};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "Nothing should match me");
    std::vector<CollisionProxy*> results1 = collision_manager.searchOpenMp(query1);
    EXPECT_EQ(results1.size(), 0);

    Query query2 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
    std::vector<CollisionProxy*> results2 = collision_manager.searchOpenMp(query2);
    EXPECT_EQ(results2.size(), 1);
    EXPECT_EQ(*results2[0]->borough, "BROOKLYN");

    Query query3 = Query::create(CollisionField::BOROUGH, Qualifier::NOT, QueryType::EQUALS, "BROOKLYN");
    std::vector<CollisionProxy*> results3 = collision_manager.searchOpenMp(query3);
    EXPECT_EQ(results3.size(), 1);
    EXPECT_EQ(*results3[0]->borough, "QUEENS");
}

TEST_F(CollisionManagerTest, MatchCaseInsensitive) {
    Collision collision1{};
    collision1.borough = "BROOKLYN";

    Collision collision2{};
    collision2.borough = "QUEENS";

    std::vector<Collision> collisions{collision1, collision2};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "Nothing should match me");
    std::vector<CollisionProxy*> results1 = collision_manager.searchOpenMp(query1);
    EXPECT_EQ(results1.size(), 0);

    Query query2 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "BROOKLYN");
    std::vector<CollisionProxy*> results2 = collision_manager.searchOpenMp(query2);
    EXPECT_EQ(results2.size(), 1);
    EXPECT_EQ(*results2[0]->borough, "BROOKLYN");

    Query query3 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, "brooklyn", Qualifier::CASE_INSENSITIVE);
    std::vector<CollisionProxy*> results3 = collision_manager.searchOpenMp(query3);
    EXPECT_EQ(results3.size(), 1);
    EXPECT_EQ(*results3[0]->borough, "BROOKLYN");

}

TEST_F(CollisionManagerTest, MatchEqualsDate) {
    Collision collision1{};
    std::chrono::year_month_day date{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };
    collision1.crash_date = date;

    std::vector<Collision> collisions{collision1};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query1 = Query::create(CollisionField::CRASH_DATE, QueryType::EQUALS, date);
    std::vector<CollisionProxy*> results1 = collision_manager.searchOpenMp(query1);
    EXPECT_EQ(results1.size(), 1);
}

TEST_F(CollisionManagerTest, MatchGreaterThanDate) {
    Collision collision1{} , collision2{};
    std::chrono::year_month_day date1{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };

    std::chrono::year_month_day date2{
        std::chrono::year{2022},
        std::chrono::month{3},
        std::chrono::day{26}
    };

    collision1.crash_date = date1;
    collision2.crash_date = date2;

    std::vector<Collision> collisions{collision1,collision2};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query = Query::create(CollisionField::CRASH_DATE, QueryType::GREATER_THAN, date1);
    std::vector<CollisionProxy*> results = collision_manager.searchOpenMp(query);
    EXPECT_EQ(results.size(), 1);
}

TEST_F(CollisionManagerTest, MatchLessThanDate) {
    Collision collision1{} , collision2{};

    std::chrono::year_month_day date1{
        std::chrono::year{2022},
        std::chrono::month{3},
        std::chrono::day{26}
    };

    std::chrono::year_month_day date2{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };

    collision1.crash_date = date1;
    collision2.crash_date = date2;

    std::vector<Collision> collisions{collision1,collision2};

    CollisionManager collision_manager = create_collision_manager(collisions);

    Query query = Query::create(CollisionField::CRASH_DATE, QueryType::LESS_THAN, date1);
    std::vector<CollisionProxy*> results = collision_manager.searchOpenMp(query);
    EXPECT_EQ(results.size(), 1);
}

TEST_F(CollisionManagerTest, CSV_Query_MatchLessThanDate) {

    std::chrono::year_month_day date1{
        std::chrono::year{2022},
        std::chrono::month{3},
        std::chrono::day{26}
    };

    Query query = Query::create(CollisionField::CRASH_DATE, QueryType::LESS_THAN, date1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(*collision->crash_date < date1)
            << "Each result should have date less than " << date1;
    }

    std::cout << "Found " << results.size() << " collisions before " << date1 << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchGreaterThanDate) {

    std::chrono::year_month_day date1{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };

    Query query = Query::create(CollisionField::CRASH_DATE, QueryType::GREATER_THAN, date1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(*collision->crash_date > date1)
            << "Each result should have date greater than " << date1;
    }

    std::cout << "Found " << results.size() << " collisions after " << date1 << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchEqualsDate) {

    std::chrono::year_month_day date1{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };

    Query query = Query::create(CollisionField::CRASH_DATE, QueryType::EQUALS, date1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(*collision->crash_date == date1)
            << "Each result should have date greater than " << date1;
    }

    std::cout << "Found " << results.size() << " collisions on " << date1 << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchEqualsTime) {

    std::chrono::hh_mm_ss<std::chrono::minutes> time1{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query = Query::create(CollisionField::CRASH_TIME, QueryType::EQUALS, time1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->crash_time->has_value() && collision->crash_time->value().to_duration() == time1.to_duration())
            << "Each result should have time equal to " << time1;
    }

    std::cout << "Found " << results.size() << " collisions on " << time1 << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchGreaterThanTime) {

    std::chrono::hh_mm_ss<std::chrono::minutes> time1{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query = Query::create(CollisionField::CRASH_TIME, QueryType::GREATER_THAN, time1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->crash_time->has_value() && collision->crash_time->value().to_duration() > time1.to_duration())
            << "Each result should have time equal to " << time1;
    }

    std::cout << "Found " << results.size() << " collisions after " << time1 << std::endl;
}


TEST_F(CollisionManagerTest, CSV_Query_MatchLessThanTime) {

    std::chrono::hh_mm_ss<std::chrono::minutes> time1{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query = Query::create(CollisionField::CRASH_TIME, QueryType::LESS_THAN, time1);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->crash_time->has_value() && collision->crash_time->value().to_duration() < time1.to_duration())
            << "Each result should have time equal to " << time1;
    }

    std::cout << "Found " << results.size() << " collisions before " << time1 << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchEqualLatitude) {

    float latitude = 40.667202f;

    Query query = Query::create(CollisionField::LATITUDE, QueryType::EQUALS, latitude);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results)
    {
       EXPECT_TRUE(collision->latitude->has_value());
       EXPECT_NEAR(collision->latitude->value(), latitude,0.001f)
            << "Latitude values should be equal within floating-point precision";
    }

    std::cout << "Found " << results.size() << " collisions at the latitude " << latitude << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchGreaterLatitude) {

    float latitude = 40.667202f;

    Query query = Query::create(CollisionField::LATITUDE, QueryType::GREATER_THAN, latitude);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results)
    {
       EXPECT_TRUE(collision->latitude->has_value());
       EXPECT_GT(collision->latitude->value(), latitude)
            << "Latitude values should be equal within floating-point precision";
    }

    std::cout << "Found " << results.size() << " collisions above the latitude " << latitude << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchLesserThanLatitude) {

    float latitude = 40.667202f;

    Query query = Query::create(CollisionField::LATITUDE, QueryType::LESS_THAN, latitude);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results)
    {
       EXPECT_TRUE(collision->latitude->has_value());
       EXPECT_LT(collision->latitude->value(), latitude)
            << "Latitude values should be equal within floating-point precision";
    }

    std::cout << "Found " << results.size() << " collisions below the latitude " << latitude << std::endl;
}

TEST_F(CollisionManagerTest, CSV_Query_MatchEqualsZipcode) {

    uint32_t zip_code = 11208;

    Query query = Query::create(CollisionField::ZIP_CODE, QueryType::EQUALS, zip_code);
    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->zip_code->value() == zip_code)
            << "Each result should have zip_code equal to " << zip_code;
    }

    std::cout << "Found " << results.size() << " collisions on " << zip_code << std::endl;
}

TEST_F(CollisionManagerTest, CompoundQuery_Match_EqualsBorough_and_GreaterThanTime) {

    std::string borough = "BROOKLYN";
    std::chrono::hh_mm_ss<std::chrono::minutes> crash_time{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, borough).add(CollisionField::CRASH_TIME, QueryType::GREATER_THAN, crash_time);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->borough->value() == borough && collision->crash_time->has_value() && collision->crash_time->value().to_duration() > crash_time.to_duration())
            << "Each result should have borough equal to " << borough << " and " << "crash time greater than " << crash_time;
    }

    std::cout << "Found " << results.size() << " collisions at " << borough << " after time " << crash_time << std::endl;

}

TEST_F(CollisionManagerTest, CompoundQuery_Match_EqualsBorough_and_LesserThanTime) {

    std::string borough = "BROOKLYN";
    std::chrono::hh_mm_ss<std::chrono::minutes> crash_time{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, borough).add(CollisionField::CRASH_TIME, QueryType::LESS_THAN, crash_time);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->borough->value() == borough && collision->crash_time->has_value() && collision->crash_time->value().to_duration() < crash_time.to_duration())
            << "Each result should have borough equal to " << borough << " and " << "crash time lesser than " << crash_time;
    }

    std::cout << "Found " << results.size() << " collisions at " << borough << " before time " << crash_time << std::endl;

}

TEST_F(CollisionManagerTest, CompoundQuery_Match_EqualsZipCode_and_GreaterThanTime) {

    uint32_t zip_code = 11208;
    std::chrono::hh_mm_ss<std::chrono::minutes> crash_time{
        std::chrono::hours{9} + std::chrono::minutes{35}
    };

    Query query1 = Query::create(CollisionField::ZIP_CODE, QueryType::EQUALS, zip_code).add(CollisionField::CRASH_TIME, QueryType::GREATER_THAN, crash_time);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->zip_code->value() == zip_code && collision->crash_time->has_value() && collision->crash_time->value().to_duration() > crash_time.to_duration())
            << "Each result should have zip_code equal to " << zip_code << " and " << "crash time greater than " << crash_time;
    }

    std::cout << "Found " << results.size() << " collisions at " << zip_code << " before time " << crash_time << std::endl;
}


TEST_F(CollisionManagerTest, CompoundQuery_Match_EqualsZipCode_and_LesserThanTime) {

    uint32_t zip_code = 11434;
    std::chrono::hh_mm_ss<std::chrono::minutes> crash_time{
        std::chrono::hours{16} + std::chrono::minutes{50}
    };

    Query query1 = Query::create(CollisionField::ZIP_CODE, QueryType::EQUALS, zip_code).add(CollisionField::CRASH_TIME, QueryType::LESS_THAN, crash_time);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->zip_code->value() == zip_code && collision->crash_time->has_value() && collision->crash_time->value().to_duration() < crash_time.to_duration())
            << "Each result should have zip_code equal to " << zip_code << " and " << "crash time less than " << crash_time;
    }

    std::cout << "Found " << results.size() << " collisions at " << zip_code << " before time " << crash_time << std::endl;
}

TEST_F(CollisionManagerTest, CompoundQuery_Match_MutipleFields) {

    std::chrono::year_month_day date1{
        std::chrono::year{2021},
        std::chrono::month{4},
        std::chrono::day{14}
    };

    std::chrono::year_month_day date2{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };

    std::chrono::hh_mm_ss<std::chrono::minutes> crash_time{
        std::chrono::hours{4} + std::chrono::minutes{45}
    };

    std::string borough = "MANHATTAN";

    uint8_t persons_injured = 0;

    Query query1 = Query::create(CollisionField::CRASH_DATE, QueryType::GREATER_THAN, date1)
    .add(CollisionField::CRASH_DATE, QueryType::LESS_THAN, date2)
    .add(CollisionField::CRASH_TIME, QueryType::GREATER_THAN, crash_time)
    .add(CollisionField::BOROUGH, QueryType::EQUALS, borough)
    .add(CollisionField::NUMBER_OF_PERSONS_INJURED, QueryType::GREATER_THAN, persons_injured);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results) {
        EXPECT_TRUE(collision->crash_date->value() > date1 && collision->crash_date->value() < date2 &&
        collision->borough->value() == "MANHATTAN" &&
        collision->crash_time->has_value() && collision->crash_time->value().to_duration() > crash_time.to_duration() &&
        collision->number_of_persons_injured->value() > persons_injured)
            << "Each result should have dates in between " << date1 << " and " << date2 << " . The crash time is after " << crash_time
            << " . Collisions occurred at borough " << borough << " and number of people injured are " << persons_injured;
    }

    std::cout << "Found " << results.size() << " collisions at " << borough << " between dates " << date1 << " and " <<  date2 <<
    " occuring after time " << crash_time << " injuring " << persons_injured << " people. " << std::endl;

}


TEST_F(CollisionManagerTest, CompoundQuery_Match_MutipleCollisions) {

    std::string contributing_factor_vehicle_2 = "Unspecified";
    std::string vehicle_type_code_1 = "Bus";
    std::string vehicle_type_code_2 = "Station Wagon/";
    std::string borough = "brooklyn";

    std::chrono::year_month_day date1{
        std::chrono::year{2021},
        std::chrono::month{9},
        std::chrono::day{11}
    };
    std::chrono::year_month_day date2{
        std::chrono::year{2022},
        std::chrono::month{6},
        std::chrono::day{29}
    };

    Query query1 = Query::create(CollisionField::BOROUGH, QueryType::EQUALS, borough, Qualifier::CASE_INSENSITIVE)
    .add(CollisionField::CRASH_DATE, QueryType::GREATER_THAN, date1)
    .add(CollisionField::CRASH_DATE, QueryType::LESS_THAN, date2)
    .add(CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2, QueryType::EQUALS, contributing_factor_vehicle_2)
    .add(CollisionField::VEHICLE_TYPE_CODE_1, QueryType::EQUALS, vehicle_type_code_1)
    .add(CollisionField::VEHICLE_TYPE_CODE_2, QueryType::CONTAINS, vehicle_type_code_2);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for (const auto *collision : results)
    {
        EXPECT_TRUE(collision->borough->value() == borough &&
        collision->crash_date->value() > date1 && collision->crash_date->value() < date2 &&
        collision->contributing_factor_vehicle_2->value() == contributing_factor_vehicle_2 &&
        collision->vehicle_type_code_1->value() == vehicle_type_code_1 || collision->vehicle_type_code_2->has_value() && std::string(collision->vehicle_type_code_2->value().c_str()).find(vehicle_type_code_2) != std::string::npos)
            << "Each result should have dates in between " << date1 << " and " << date2 << " . The contributing factor to the collisions is anything " << contributing_factor_vehicle_2
            << " . The vehicles involved are " << vehicle_type_code_1 << " and " << vehicle_type_code_2;
    }

    std::cout << "Found " << results.size() << " collisions at " << borough << " between dates " << date1 << " and " <<  date2 <<
    " . The contributing factor to the collisions is  " << contributing_factor_vehicle_2 << " . The vehicles involved are " << vehicle_type_code_1 << " and "<< vehicle_type_code_2 << std::endl;
}

TEST_F(CollisionManagerTest, Query_Match_VehicleType) {

    std::string vehicle_type_code_2 = "Station Wagon";
    Query query1 = Query::create(CollisionField::VEHICLE_TYPE_CODE_2, QueryType::CONTAINS, vehicle_type_code_2);

    std::vector<CollisionProxy*> results = collision_manager_m.searchOpenMp(query1);

    EXPECT_GT(results.size(), 0) << "Search should return at least one result";

    for(const auto *collision : results) {
        EXPECT_TRUE(collision->vehicle_type_code_2->has_value() && std::string(collision->vehicle_type_code_2->value().c_str()).find(vehicle_type_code_2) != std::string::npos) << " Each result should contain " << vehicle_type_code_2;
    }

    std::cout << " Found " << results.size() << " with vehicle_type_code_2 containing " << vehicle_type_code_2;

}

