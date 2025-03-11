#pragma once

#include "query.hpp"

#include <chrono>
#include <iostream>
#include <format>
#include <optional>
#include <string>


struct Collision {
    std::optional<std::chrono::year_month_day> crash_date;
    std::optional<std::chrono::hh_mm_ss<std::chrono::minutes>> crash_time;
    std::optional<std::string> borough;
    std::optional<std::uint32_t> zip_code;
    std::optional<float> latitude;
    std::optional<float> longitude;
    std::optional<std::string> location;
    std::optional<std::string> on_street_name;
    std::optional<std::string> cross_street_name;
    std::optional<std::string> off_street_name;
    std::optional<std::uint8_t> number_of_persons_injured;
    std::optional<std::uint8_t> number_of_persons_killed;
    std::optional<std::uint8_t> number_of_pedestrians_injured;
    std::optional<std::uint8_t> number_of_pedestrians_killed;
    std::optional<std::uint8_t> number_of_cyclist_injured;
    std::optional<std::uint8_t> number_of_cyclist_killed;
    std::optional<std::uint8_t> number_of_motorist_injured;
    std::optional<std::uint8_t> number_of_motorist_killed;
    std::optional<std::string> contributing_factor_vehicle_1;
    std::optional<std::string> contributing_factor_vehicle_2;
    std::optional<std::string> contributing_factor_vehicle_3;
    std::optional<std::string> contributing_factor_vehicle_4;
    std::optional<std::string> contributing_factor_vehicle_5;
    std::optional<std::size_t> collision_id;
    std::optional<std::string> vehicle_type_code_1;
    std::optional<std::string> vehicle_type_code_2;
    std::optional<std::string> vehicle_type_code_3;
    std::optional<std::string> vehicle_type_code_4;
    std::optional<std::string> vehicle_type_code_5;
};

struct CollisionProxy {
    std::optional<std::chrono::year_month_day>* crash_date;
    std::optional<std::chrono::hh_mm_ss<std::chrono::minutes>>* crash_time;
    std::optional<std::string>* borough;
    std::optional<std::uint32_t>* zip_code;
    std::optional<float>* latitude;
    std::optional<float>* longitude;
    std::optional<std::string>* location;
    std::optional<std::string>* on_street_name;
    std::optional<std::string>* cross_street_name;
    std::optional<std::string>* off_street_name;
    std::optional<std::uint8_t>* number_of_persons_injured;
    std::optional<std::uint8_t>* number_of_persons_killed;
    std::optional<std::uint8_t>* number_of_pedestrians_injured;
    std::optional<std::uint8_t>* number_of_pedestrians_killed;
    std::optional<std::uint8_t>* number_of_cyclist_injured;
    std::optional<std::uint8_t>* number_of_cyclist_killed;
    std::optional<std::uint8_t>* number_of_motorist_injured;
    std::optional<std::uint8_t>* number_of_motorist_killed;
    std::optional<std::string>* contributing_factor_vehicle_1;
    std::optional<std::string>* contributing_factor_vehicle_2;
    std::optional<std::string>* contributing_factor_vehicle_3;
    std::optional<std::string>* contributing_factor_vehicle_4;
    std::optional<std::string>* contributing_factor_vehicle_5;
    std::optional<std::size_t>* collision_id;
    std::optional<std::string>* vehicle_type_code_1;
    std::optional<std::string>* vehicle_type_code_2;
    std::optional<std::string>* vehicle_type_code_3;
    std::optional<std::string>* vehicle_type_code_4;
    std::optional<std::string>* vehicle_type_code_5;
};

struct Collisions {
    std::vector<std::optional<std::chrono::year_month_day>> crash_dates;
    std::vector<std::optional<std::chrono::hh_mm_ss<std::chrono::minutes>>> crash_times;
    std::vector<std::optional<std::string>> boroughs;
    std::vector<std::optional<std::uint32_t>> zip_codes;
    std::vector<std::optional<float>> latitudes;
    std::vector<std::optional<float>> longitudes;
    std::vector<std::optional<std::string>> locations;
    std::vector<std::optional<std::string>> on_street_names;
    std::vector<std::optional<std::string>> cross_street_names;
    std::vector<std::optional<std::string>> off_street_names;
    std::vector<std::optional<std::uint8_t>> numbers_of_persons_injured;
    std::vector<std::optional<std::uint8_t>> numbers_of_persons_killed;
    std::vector<std::optional<std::uint8_t>> numbers_of_pedestrians_injured;
    std::vector<std::optional<std::uint8_t>> numbers_of_pedestrians_killed;
    std::vector<std::optional<std::uint8_t>> numbers_of_cyclist_injured;
    std::vector<std::optional<std::uint8_t>> numbers_of_cyclist_killed;
    std::vector<std::optional<std::uint8_t>> numbers_of_motorist_injured;
    std::vector<std::optional<std::uint8_t>> numbers_of_motorist_killed;
    std::vector<std::optional<std::string>> contributing_factor_vehicles_1;
    std::vector<std::optional<std::string>> contributing_factor_vehicles_2;
    std::vector<std::optional<std::string>> contributing_factor_vehicles_3;
    std::vector<std::optional<std::string>> contributing_factor_vehicles_4;
    std::vector<std::optional<std::string>> contributing_factor_vehicles_5;
    std::vector<std::optional<std::size_t>> collision_ids;
    std::vector<std::optional<std::string>> vehicle_type_codes_1;
    std::vector<std::optional<std::string>> vehicle_type_codes_2;
    std::vector<std::optional<std::string>> vehicle_type_codes_3;
    std::vector<std::optional<std::string>> vehicle_type_codes_4;
    std::vector<std::optional<std::string>> vehicle_type_codes_5;

    void add(const Collision& collision);
    void combine(const Collisions& other);
    std::size_t size();

private:
    std::size_t size_;
};

class IndexedCollisions {
public:
    IndexedCollisions();
    IndexedCollisions(Collisions& collisions);

    // Underlying data from csv
    Collisions collisions_;

    // For API to return CollisionProxy* instances instead of just integer indexes into data
    std::vector<CollisionProxy> proxies_;
    std::vector<CollisionProxy*> proxy_ptrs_;

    // Sorted indexes by various fields for fast queries
    std::vector<std::uint32_t> sorted_crash_dates;
    std::vector<std::uint32_t> sorted_crash_times;
    std::vector<std::uint32_t> sorted_zip_codes;
    std::vector<std::uint32_t> sorted_latitudes;
    std::vector<std::uint32_t> sorted_longitudes;
    std::vector<std::uint32_t> sorted_numbers_of_persons_injured;
    std::vector<std::uint32_t> sorted_numbers_of_persons_killed;
    std::vector<std::uint32_t> sorted_numbers_of_pedestrians_injured;
    std::vector<std::uint32_t> sorted_numbers_of_pedestrians_killed;
    std::vector<std::uint32_t> sorted_numbers_of_cyclist_injured;
    std::vector<std::uint32_t> sorted_numbers_of_cyclist_killed;
    std::vector<std::uint32_t> sorted_numbers_of_motorist_injured;
    std::vector<std::uint32_t> sorted_numbers_of_motorist_killed;
    std::vector<std::uint32_t> sorted_collision_ids;

    void match(const FieldQuery& query,
               const std::size_t start_index,
               const std::size_t end_index,
               std::span<std::uint8_t> matches) const;

private:
    void init_proxies();
    void init_indexes();
    const CollisionProxy index_to_collision(const std::size_t index);
};

std::ostream& operator<<(std::ostream& os, const CollisionProxy& collision);
