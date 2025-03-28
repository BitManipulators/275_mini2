#include "collision.hpp"

#include "query.hpp"

#include <chrono>
#include <iostream>
#include <format>
#include <numeric>
#include <omp.h>
#include <optional>
#include <string>

template<class T>
bool equals_is_less_than(const FieldQuery& query, const std::optional<T>& value) {
    const QueryType& type = query.get_type();
    const T& query_value = std::get<T>(query.get_value());

    if (!value.has_value()) {
        return false;
    }

    if constexpr (std::is_same_v<float, T> || std::is_same_v<std::size_t, T> || std::is_same_v<std::chrono::year_month_day, T> ||
                  std::is_same_v<std::uint8_t, T> || std::is_same_v<std::uint32_t, T>) {
        switch(type) {
        case QueryType::EQUALS:
            return *value < query_value;
        default:
            throw std::runtime_error("Unsupported QueryType for float/std::size_t/std::chrono::year_month_day/std::uint8_t/std::uint32_t");
        }
    }

    throw std::runtime_error("Unsupported value type");
}

template<class T>
bool do_match(const FieldQuery& query, const std::optional<T>& value) {
    const QueryType& type = query.get_type();

    if (type == QueryType::HAS_VALUE) {
        return value.has_value();
    }

    if (!value.has_value()) {
        return false;
    }

    const T& query_value = std::get<T>(query.get_value());

    if constexpr (std::is_same_v<float, T> || std::is_same_v<std::size_t, T> || std::is_same_v<std::chrono::year_month_day, T> ||
                  std::is_same_v<std::uint8_t, T> || std::is_same_v<std::uint32_t, T>) {
        switch(type) {
        case QueryType::EQUALS:
            return *value == query_value;
        case QueryType::LESS_THAN:
            return *value < query_value;
        case QueryType::GREATER_THAN:
            return *value > query_value;
        case QueryType::CONTAINS:
        default:
            throw std::runtime_error("Unsupported QueryType for float/std::size_t/std::chrono::year_month_day/std::uint8_t/std::uint32_t");
        }
    } else if constexpr (std::is_same_v<std::chrono::hh_mm_ss<std::chrono::minutes>, T>) {
        switch(type) {
        case QueryType::EQUALS:
            return (*value).to_duration() == query_value.to_duration();
        case QueryType::LESS_THAN:
            return (*value).to_duration() < query_value.to_duration();
        case QueryType::GREATER_THAN:
            return (*value).to_duration() > query_value.to_duration();
        case QueryType::CONTAINS:
        default:
            throw std::runtime_error("Unsupported QueryType for std::chrono::hh_mm_ss");
        }
    } else if constexpr (std::is_same_v<std::string, T> || std::is_same_v<CollisionString, T>) {
        std::string first_value;
        std::string second_value;
        if constexpr (std::is_same_v<std::string, T>) {
            first_value = *value;
            second_value = query_value;
        } else if constexpr (std::is_same_v<CollisionString, T>) {
            first_value = std::string((*value).data, (*value).length);
            second_value = std::string(query_value.data, query_value.length);
        }

        if (query.case_insensitive()) {
            std::transform(first_value.begin(), first_value.end(), first_value.begin(), ::tolower);
            std::transform(second_value.begin(), second_value.end(), second_value.begin(), ::tolower);
        }

        switch(type) {
        case QueryType::EQUALS:
            return first_value == second_value;
        case QueryType::CONTAINS:
            return first_value.find(second_value) != std::string::npos;
        case QueryType::LESS_THAN:
        case QueryType::GREATER_THAN:
        default:
            throw std::runtime_error("Unsupported QueryType for std::string");
        }
    } else {
        static_assert(false, "Unsupported type, Only float, std::size_t, std::string, std::chrono::year_month_day, and std::chrono::hh_mm_ss, std::uint8_t, std::uint32_t types are allowed.");
    }

    return false;
}

template<class T>
void match_field(const FieldQuery& query,
                 const std::size_t start_index,
                 const std::size_t end_index,
                 const std::vector<T>& items,
                 std::span<std::uint8_t>& matches_span) {
    std::uint8_t* matches = matches_span.data();
    for (std::size_t index = 0; index < matches_span.size(); ++index) {
        if (*matches) {
            bool match = do_match(query, items[start_index + index]);
            match = query.invert_match() ? !match : match;
            if (!match) {
                *matches = false;
            }
        }
        ++matches;
    }
}

// Start: AI Generated Binary Search Code
// ======================================
template<class T>
const std::uint32_t* binary_search_find_first_lower_match(const FieldQuery& query,
                                                          const std::size_t start_index,
                                                          const std::size_t end_index,
                                                          const std::vector<T>& items,
                                                          const std::vector<std::uint32_t>& items_index) {
    int low = start_index;
    int high = end_index - 1;
    const std::uint32_t* result = nullptr;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (do_match(query, items[items_index[mid]])) {
            result = &items_index[mid];
            high = mid - 1;
        } else {
            if (query.get_type() == QueryType::EQUALS) {
                if (equals_is_less_than(query, items[items_index[mid]])) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            } else if (query.get_type() == QueryType::LESS_THAN) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
    }

    return result;
}

template<class T>
const std::uint32_t* binary_search_find_last_upper_match(const FieldQuery& query,
                                                         const std::size_t start_index,
                                                         const std::size_t end_index,
                                                         const std::vector<T>& items,
                                                         const std::vector<std::uint32_t>& items_index) {
    int low = start_index;
    int high = end_index - 1;
    const std::uint32_t* result = nullptr;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (do_match(query, items[items_index[mid]])) {
            result = &items_index[mid];
            low = mid + 1;
        } else {
            if (query.get_type() == QueryType::EQUALS) {
                if (equals_is_less_than(query, items[items_index[mid]])) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            } else if (query.get_type() == QueryType::LESS_THAN) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
    }

    return result;
}
// End: AI Generated Binary Search Code
// ====================================

template<class T>
void match_indexed_field(const FieldQuery& query,
                         const std::size_t start_index,
                         const std::size_t end_index,
                         const std::vector<T>& items,
                         const std::vector<std::uint32_t>& items_index,
                         std::span<std::uint8_t>& matches_span) {

    std::uint8_t* matches = matches_span.data();

    // Using binary search we find the indexes of the first and last matching items.
    // We therefore know that everything outside this range is not a match, and do not need
    // to call match on every single item anymore.

    const std::uint32_t* lower_bound = binary_search_find_first_lower_match(
        query, start_index, end_index, items, items_index
    );
    const std::uint32_t* upper_bound = binary_search_find_last_upper_match(
        query, start_index, end_index, items, items_index
    );

    // If either lower or upper bound are null then unmatch all indexes
    if (lower_bound == nullptr || upper_bound == nullptr) {
        for (std::size_t index = 0; index < end_index - start_index; ++index) {
            *(matches + items_index[index + start_index]) = false;
        }
        return;
    }

    const std::uint32_t lower_bound_index = lower_bound - &(items_index.begin() + start_index)[0];
    const std::uint32_t upper_bound_index = upper_bound - &(items_index.begin() + start_index)[0] + 1;

    // Unmatch anything before lower_bound
    for (std::size_t index = 0; index < lower_bound_index; ++index) {
        *(matches + items_index[index + start_index]) = false;
    }

    // Unmatch anything after upper_bound
    for (std::size_t index = upper_bound_index; index < end_index - start_index; ++index) {
        *(matches + items_index[index + start_index]) = false;
    }
}

IndexedCollisions::IndexedCollisions(Collisions& collisions)
  : collisions_{collisions}
{
    init_proxies();
    init_indexes();
}

IndexedCollisions::IndexedCollisions()
  : collisions_{}
{
}

const CollisionProxy IndexedCollisions::index_to_collision(const std::size_t index) {
    CollisionProxy collision{};
    collision.crash_date = &(collisions_.crash_dates[index]);
    collision.crash_time = &(collisions_.crash_times[index]);
    collision.borough = &(collisions_.boroughs[index]);
    collision.zip_code = &(collisions_.zip_codes[index]);
    collision.latitude = &(collisions_.latitudes[index]);
    collision.longitude = &(collisions_.longitudes[index]);
    collision.location = &(collisions_.locations[index]);
    collision.on_street_name = &(collisions_.on_street_names[index]);
    collision.cross_street_name = &(collisions_.cross_street_names[index]);
    collision.off_street_name = &(collisions_.off_street_names[index]);
    collision.number_of_persons_injured = &(collisions_.numbers_of_persons_injured[index]);
    collision.number_of_persons_killed = &(collisions_.numbers_of_persons_killed[index]);
    collision.number_of_pedestrians_injured = &(collisions_.numbers_of_pedestrians_injured[index]);
    collision.number_of_pedestrians_killed = &(collisions_.numbers_of_pedestrians_killed[index]);
    collision.number_of_cyclist_injured = &(collisions_.numbers_of_cyclist_injured[index]);
    collision.number_of_cyclist_killed = &(collisions_.numbers_of_cyclist_killed[index]);
    collision.number_of_motorist_injured = &(collisions_.numbers_of_motorist_injured[index]);
    collision.number_of_motorist_killed = &(collisions_.numbers_of_motorist_killed[index]);
    collision.contributing_factor_vehicle_1 = &(collisions_.contributing_factor_vehicles_1[index]);
    collision.contributing_factor_vehicle_2 = &(collisions_.contributing_factor_vehicles_2[index]);
    collision.contributing_factor_vehicle_3 = &(collisions_.contributing_factor_vehicles_3[index]);
    collision.contributing_factor_vehicle_4 = &(collisions_.contributing_factor_vehicles_4[index]);
    collision.contributing_factor_vehicle_5 = &(collisions_.contributing_factor_vehicles_5[index]);
    collision.collision_id = &(collisions_.collision_ids[index]);
    collision.vehicle_type_code_1 = &(collisions_.vehicle_type_codes_1[index]);
    collision.vehicle_type_code_2 = &(collisions_.vehicle_type_codes_2[index]);
    collision.vehicle_type_code_3 = &(collisions_.vehicle_type_codes_3[index]);
    collision.vehicle_type_code_4 = &(collisions_.vehicle_type_codes_4[index]);
    collision.vehicle_type_code_5 = &(collisions_.vehicle_type_codes_5[index]);
    return collision;
}

void IndexedCollisions::init_proxies() {
    this->proxies_ = {};
    for (std::size_t index = 0; index < collisions_.size(); ++index) {
        proxies_.push_back(index_to_collision(index));
    }

    this->proxy_ptrs_ = {};
    for (std::size_t index = 0; index < collisions_.size(); ++index) {
        proxy_ptrs_.push_back(&proxies_[index]);
    }
}

template<class T>
void init_index(T& vec_data, std::vector<uint32_t>& sorted_indexes) {
    sorted_indexes = std::vector<uint32_t>(vec_data.size());
    std::iota(sorted_indexes.begin(), sorted_indexes.end(), 0);
    std::sort(sorted_indexes.begin(), sorted_indexes.end(), [&vec_data, &sorted_indexes](const uint32_t first, const uint32_t second) {
        if (!vec_data[first].has_value() && !vec_data[second].has_value()) {
            return false;
        } else if (!vec_data[first].has_value()) {
            return false;
        } else if (!vec_data[second].has_value()) {
            return true;
        } else {
            return vec_data[first].value() < vec_data[second].value();
        }});
}

void IndexedCollisions::init_indexes() {
    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            {
                init_index(collisions_.crash_dates, sorted_crash_dates);
            }
            #pragma omp task
            {
                init_index(collisions_.zip_codes, sorted_zip_codes);
            }
            #pragma omp task
            {
                init_index(collisions_.latitudes, sorted_latitudes);
            }
            #pragma omp task
            {
                init_index(collisions_.longitudes, sorted_longitudes);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_persons_injured, sorted_numbers_of_persons_injured);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_persons_killed, sorted_numbers_of_persons_killed);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_pedestrians_injured, sorted_numbers_of_pedestrians_injured);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_pedestrians_killed, sorted_numbers_of_pedestrians_killed);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_cyclist_injured, sorted_numbers_of_cyclist_injured);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_cyclist_killed, sorted_numbers_of_cyclist_killed);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_motorist_injured, sorted_numbers_of_motorist_injured);
            }
            #pragma omp task
            {
                init_index(collisions_.numbers_of_motorist_killed, sorted_numbers_of_motorist_killed);
            }
            #pragma omp task
            {
                init_index(collisions_.collision_ids, sorted_collision_ids);
            }
        }
    }

// TODO: support crash_times (requires converting all entries to durations for sorting comparisons to work)
//    init_index(collisions_.crash_times, sorted_crash_times);
}

void IndexedCollisions::match(const FieldQuery& query,
                       const std::size_t start_index,
                       const std::size_t end_index,
                       std::span<std::uint8_t> matches) const {
    const CollisionField& name = query.get_name();

    std::span<std::uint8_t> matches_span;
    if (is_indexed_field(name)) {
        // Send whole matches vector to match function
        // because the indexes can reference anywhere in the global collisions
        matches_span = matches;
    } else {
        // Send subset of matches vector to match function
        // because the indexes operated on will be inside the chunk only
        matches_span = {matches.data() + start_index, end_index - start_index};
    }

    if (name == CollisionField::CRASH_DATE) {
        match_indexed_field(query, start_index, end_index, collisions_.crash_dates, sorted_crash_dates, matches_span);
    } else if (name == CollisionField::CRASH_TIME) {
        match_field(query, start_index, end_index, collisions_.crash_times, matches_span);
    } else if (name == CollisionField::BOROUGH) {
        match_field(query, start_index, end_index, collisions_.boroughs, matches_span);
    } else if (name == CollisionField::ZIP_CODE) {
        match_indexed_field(query, start_index, end_index, collisions_.zip_codes, sorted_zip_codes, matches_span);
    } else if (name == CollisionField::LATITUDE) {
        match_indexed_field(query, start_index, end_index, collisions_.latitudes, sorted_latitudes, matches_span);
    } else if (name == CollisionField::LONGITUDE) {
        match_indexed_field(query, start_index, end_index, collisions_.longitudes, sorted_longitudes, matches_span);
    } else if (name == CollisionField::LOCATION) {
        match_field(query, start_index, end_index, collisions_.locations, matches_span);
    } else if (name == CollisionField::ON_STREET_NAME) {
        match_field(query, start_index, end_index, collisions_.on_street_names, matches_span);
    } else if (name == CollisionField::CROSS_STREET_NAME) {
        match_field(query, start_index, end_index, collisions_.cross_street_names, matches_span);
    } else if (name == CollisionField::OFF_STREET_NAME) {
        match_field(query, start_index, end_index, collisions_.off_street_names, matches_span);
    } else if (name == CollisionField::NUMBER_OF_PERSONS_INJURED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_persons_injured, sorted_numbers_of_persons_injured, matches_span);
    } else if (name == CollisionField::NUMBER_OF_PERSONS_KILLED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_persons_killed, sorted_numbers_of_persons_killed, matches_span);
    } else if (name == CollisionField::NUMBER_OF_PEDESTRIANS_INJURED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_pedestrians_injured, sorted_numbers_of_pedestrians_injured, matches_span);
    } else if (name == CollisionField::NUMBER_OF_PEDESTRIANS_KILLED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_pedestrians_killed, sorted_numbers_of_pedestrians_killed, matches_span);
    } else if (name == CollisionField::NUMBER_OF_CYCLIST_INJURED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_cyclist_injured, sorted_numbers_of_cyclist_injured, matches_span);
    } else if (name == CollisionField::NUMBER_OF_CYCLIST_KILLED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_cyclist_killed, sorted_numbers_of_cyclist_killed, matches_span);
    } else if (name == CollisionField::NUMBER_OF_MOTORIST_INJURED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_motorist_injured, sorted_numbers_of_motorist_injured, matches_span);
    } else if (name == CollisionField::NUMBER_OF_MOTORIST_KILLED) {
        match_indexed_field(query, start_index, end_index, collisions_.numbers_of_motorist_killed, sorted_numbers_of_motorist_killed, matches_span);
    } else if (name == CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1) {
        match_field(query, start_index, end_index, collisions_.contributing_factor_vehicles_1, matches_span);
    } else if (name == CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2) {
        match_field(query, start_index, end_index, collisions_.contributing_factor_vehicles_2, matches_span);
    } else if (name == CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3) {
        match_field(query, start_index, end_index, collisions_.contributing_factor_vehicles_3, matches_span);
    } else if (name == CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4) {
        match_field(query, start_index, end_index, collisions_.contributing_factor_vehicles_4, matches_span);
    } else if (name == CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5) {
        match_field(query, start_index, end_index, collisions_.contributing_factor_vehicles_5, matches_span);
    } else if (name == CollisionField::COLLISION_ID) {
        match_indexed_field(query, start_index, end_index, collisions_.collision_ids, sorted_collision_ids, matches_span);
    } else if (name == CollisionField::VEHICLE_TYPE_CODE_1) {
        match_field(query, start_index, end_index, collisions_.vehicle_type_codes_1, matches_span);
    } else if (name == CollisionField::VEHICLE_TYPE_CODE_2) {
        match_field(query, start_index, end_index, collisions_.vehicle_type_codes_2, matches_span);
    } else if (name == CollisionField::VEHICLE_TYPE_CODE_3) {
        match_field(query, start_index, end_index, collisions_.vehicle_type_codes_3, matches_span);
    } else if (name == CollisionField::VEHICLE_TYPE_CODE_4) {
        match_field(query, start_index, end_index, collisions_.vehicle_type_codes_4, matches_span);
    } else if (name == CollisionField::VEHICLE_TYPE_CODE_5) {
        match_field(query, start_index, end_index, collisions_.vehicle_type_codes_5, matches_span);
    }
}

std::ostream& operator<<(std::ostream& os, const CollisionProxy& collision) {
    os << "Collision: {";

    os << std::format("crash_date = {}", collision.crash_date->has_value() ?
        std::format("{:%m/%d/%Y}", collision.crash_date->value()) : "(no value)") << ", ";
    os << std::format("crash_time = {}", collision.crash_time->has_value() ?
        std::format("{:%H:%M}", collision.crash_time->value()) : "(no value)") << ", ";
    os << std::format("borough = {}", collision.borough->has_value() ?
        collision.borough->value().data : "(no value)") << ", ";
    os << std::format("zip_code = {}", collision.zip_code->has_value() ?
        std::to_string(collision.zip_code->value()) : "(no value)") << ", ";
    os << std::format("latitude = {}", collision.latitude->has_value() ?
        std::to_string(collision.latitude->value()) : "(no value)") << ", ";
    os << std::format("longitude = {}", collision.longitude->has_value() ?
        std::to_string(collision.longitude->value()) : "(no value)") << ", ";
    os << std::format("location = {}", collision.location->has_value() ?
        collision.location->value().data : "(no value)") << ", ";
    os << std::format("on_street_name = {}", collision.on_street_name->has_value() ?
        collision.on_street_name->value().data : "(no value)") << ", ";
    os << std::format("cross_street_name = {}", collision.cross_street_name->has_value() ?
        collision.cross_street_name->value().data : "(no value)") << ", ";
    os << std::format("off_street_name = {}", collision.off_street_name->has_value() ?
        collision.off_street_name->value().data : "(no value)") << ", ";
    os << std::format("number_of_persons_injured = {}", collision.number_of_persons_injured->has_value() ?
        std::to_string(collision.number_of_persons_injured->value()) : "(no value)") << ", ";
    os << std::format("number_of_persons_killed = {}", collision.number_of_persons_killed->has_value() ?
        std::to_string(collision.number_of_persons_killed->value()) : "(no value)") << ", ";
    os << std::format("number_of_pedestrians_injured = {}", collision.number_of_pedestrians_injured->has_value() ?
        std::to_string(collision.number_of_pedestrians_injured->value()) : "(no value)") << ", ";
    os << std::format("number_of_pedestrians_killed = {}", collision.number_of_pedestrians_killed->has_value() ?
        std::to_string(collision.number_of_pedestrians_killed->value()) : "(no value)") << ", ";
    os << std::format("number_of_cyclist_injured = {}", collision.number_of_cyclist_injured->has_value() ?
        std::to_string(collision.number_of_cyclist_injured->value()) : "(no value)") << ", ";
    os << std::format("number_of_cyclist_killed = {}", collision.number_of_cyclist_killed->has_value() ?
        std::to_string(collision.number_of_cyclist_killed->value()) : "(no value)") << ", ";
    os << std::format("number_of_motorist_injured = {}", collision.number_of_motorist_injured->has_value() ?
        std::to_string(collision.number_of_motorist_injured->value()) : "(no value)") << ", ";
    os << std::format("number_of_motorist_killed = {}", collision.number_of_motorist_killed->has_value() ?
        std::to_string(collision.number_of_motorist_killed->value()) : "(no value)") << ", ";
    os << std::format("contributing_factor_vehicle_1 = {}", collision.contributing_factor_vehicle_1->has_value() ?
        collision.contributing_factor_vehicle_1->value().data : "(no value)") << ", ";
    os << std::format("contributing_factor_vehicle_2 = {}", collision.contributing_factor_vehicle_2->has_value() ?
        collision.contributing_factor_vehicle_2->value().data : "(no value)") << ", ";
    os << std::format("contributing_factor_vehicle_3 = {}", collision.contributing_factor_vehicle_3->has_value() ?
        collision.contributing_factor_vehicle_3->value().data : "(no value)") << ", ";
    os << std::format("contributing_factor_vehicle_4 = {}", collision.contributing_factor_vehicle_4->has_value() ?
        collision.contributing_factor_vehicle_4->value().data : "(no value)") << ", ";
    os << std::format("contributing_factor_vehicle_5 = {}", collision.contributing_factor_vehicle_5->has_value() ?
        collision.contributing_factor_vehicle_5->value().data : "(no value)") << ", ";
    os << std::format("collision_id = {}", collision.collision_id->has_value() ?
        std::to_string(collision.collision_id->value()) : "(no value)") << ", ";
    os << std::format("vehicle_type_code_1 = {}", collision.vehicle_type_code_1->has_value() ?
        collision.vehicle_type_code_1->value().data : "(no value)") << ", ";
    os << std::format("vehicle_type_code_2 = {}", collision.vehicle_type_code_2->has_value() ?
        collision.vehicle_type_code_2->value().data : "(no value)") << ", ";
    os << std::format("vehicle_type_code_3 = {}", collision.vehicle_type_code_3->has_value() ?
        collision.vehicle_type_code_3->value().data : "(no value)") << ", ";
    os << std::format("vehicle_type_code_4 = {}", collision.vehicle_type_code_4->has_value() ?
        collision.vehicle_type_code_4->value().data : "(no value)") << ", ";
    os << std::format("vehicle_type_code_5 = {}", collision.vehicle_type_code_5->has_value() ?
        collision.vehicle_type_code_5->value().data : "(no value)") << ", ";

    os << "}";
    return os;
}

void Collisions::add(const Collision& collision) {
    crash_dates.push_back(collision.crash_date);
    crash_times.push_back(collision.crash_time);
    boroughs.push_back(collision.borough);
    zip_codes.push_back(collision.zip_code);
    latitudes.push_back(collision.latitude);
    longitudes.push_back(collision.longitude);
    locations.push_back(collision.location);
    on_street_names.push_back(collision.on_street_name);
    cross_street_names.push_back(collision.cross_street_name);
    off_street_names.push_back(collision.off_street_name);
    numbers_of_persons_injured.push_back(collision.number_of_persons_injured);
    numbers_of_persons_killed.push_back(collision.number_of_persons_killed);
    numbers_of_pedestrians_injured.push_back(collision.number_of_pedestrians_injured);
    numbers_of_pedestrians_killed.push_back(collision.number_of_pedestrians_killed);
    numbers_of_cyclist_injured.push_back(collision.number_of_cyclist_injured);
    numbers_of_cyclist_killed.push_back(collision.number_of_cyclist_killed);
    numbers_of_motorist_injured.push_back(collision.number_of_motorist_injured);
    numbers_of_motorist_killed.push_back(collision.number_of_motorist_killed);
    contributing_factor_vehicles_1.push_back(collision.contributing_factor_vehicle_1);
    contributing_factor_vehicles_2.push_back(collision.contributing_factor_vehicle_2);
    contributing_factor_vehicles_3.push_back(collision.contributing_factor_vehicle_3);
    contributing_factor_vehicles_4.push_back(collision.contributing_factor_vehicle_4);
    contributing_factor_vehicles_5.push_back(collision.contributing_factor_vehicle_5);
    collision_ids.push_back(collision.collision_id);
    vehicle_type_codes_1.push_back(collision.vehicle_type_code_1);
    vehicle_type_codes_2.push_back(collision.vehicle_type_code_2);
    vehicle_type_codes_3.push_back(collision.vehicle_type_code_3);
    vehicle_type_codes_4.push_back(collision.vehicle_type_code_4);
    vehicle_type_codes_5.push_back(collision.vehicle_type_code_5);

    size_++;
}

void Collisions::combine(const Collisions& other) {
    crash_dates.insert(crash_dates.end(), other.crash_dates.begin(), other.crash_dates.end());
    crash_times.insert(crash_times.end(), other.crash_times.begin(), other.crash_times.end());
    boroughs.insert(boroughs.end(), other.boroughs.begin(), other.boroughs.end());
    zip_codes.insert(zip_codes.end(), other.zip_codes.begin(), other.zip_codes.end());
    latitudes.insert(latitudes.end(), other.latitudes.begin(), other.latitudes.end());
    longitudes.insert(longitudes.end(), other.longitudes.begin(), other.longitudes.end());
    locations.insert(locations.end(), other.locations.begin(), other.locations.end());
    on_street_names.insert(on_street_names.end(), other.on_street_names.begin(), other.on_street_names.end());
    cross_street_names.insert(cross_street_names.end(), other.cross_street_names.begin(), other.cross_street_names.end());
    off_street_names.insert(off_street_names.end(), other.off_street_names.begin(), other.off_street_names.end());
    numbers_of_persons_injured.insert(numbers_of_persons_injured.end(), other.numbers_of_persons_injured.begin(), other.numbers_of_persons_injured.end());
    numbers_of_persons_killed.insert(numbers_of_persons_killed.end(), other.numbers_of_persons_killed.begin(), other.numbers_of_persons_killed.end());
    numbers_of_pedestrians_injured.insert(numbers_of_pedestrians_injured.end(), other.numbers_of_pedestrians_injured.begin(), other.numbers_of_pedestrians_injured.end());
    numbers_of_pedestrians_killed.insert(numbers_of_pedestrians_killed.end(), other.numbers_of_pedestrians_killed.begin(), other.numbers_of_pedestrians_killed.end());
    numbers_of_cyclist_injured.insert(numbers_of_cyclist_injured.end(), other.numbers_of_cyclist_injured.begin(), other.numbers_of_cyclist_injured.end());
    numbers_of_cyclist_killed.insert(numbers_of_cyclist_killed.end(), other.numbers_of_cyclist_killed.begin(), other.numbers_of_cyclist_killed.end());
    numbers_of_motorist_injured.insert(numbers_of_motorist_injured.end(), other.numbers_of_motorist_injured.begin(), other.numbers_of_motorist_injured.end());
    numbers_of_motorist_killed.insert(numbers_of_motorist_killed.end(), other.numbers_of_motorist_killed.begin(), other.numbers_of_motorist_killed.end());
    contributing_factor_vehicles_1.insert(contributing_factor_vehicles_1.end(), other.contributing_factor_vehicles_1.begin(), other.contributing_factor_vehicles_1.end());
    contributing_factor_vehicles_2.insert(contributing_factor_vehicles_2.end(), other.contributing_factor_vehicles_2.begin(), other.contributing_factor_vehicles_2.end());
    contributing_factor_vehicles_3.insert(contributing_factor_vehicles_3.end(), other.contributing_factor_vehicles_3.begin(), other.contributing_factor_vehicles_3.end());
    contributing_factor_vehicles_4.insert(contributing_factor_vehicles_4.end(), other.contributing_factor_vehicles_4.begin(), other.contributing_factor_vehicles_4.end());
    contributing_factor_vehicles_5.insert(contributing_factor_vehicles_5.end(), other.contributing_factor_vehicles_5.begin(), other.contributing_factor_vehicles_5.end());
    collision_ids.insert(collision_ids.end(), other.collision_ids.begin(), other.collision_ids.end());
    vehicle_type_codes_1.insert(vehicle_type_codes_1.end(), other.vehicle_type_codes_1.begin(), other.vehicle_type_codes_1.end());
    vehicle_type_codes_2.insert(vehicle_type_codes_2.end(), other.vehicle_type_codes_2.begin(), other.vehicle_type_codes_2.end());
    vehicle_type_codes_3.insert(vehicle_type_codes_3.end(), other.vehicle_type_codes_3.begin(), other.vehicle_type_codes_3.end());
    vehicle_type_codes_4.insert(vehicle_type_codes_4.end(), other.vehicle_type_codes_4.begin(), other.vehicle_type_codes_4.end());
    vehicle_type_codes_5.insert(vehicle_type_codes_5.end(), other.vehicle_type_codes_5.begin(), other.vehicle_type_codes_5.end());

    size_ = crash_dates.size();
}

std::size_t Collisions::size() {
    return size_;
}

Collision collision_proxy_to_collision(const CollisionProxy& proxy) {
    Collision collision{};
    collision.crash_date = *proxy.crash_date;
    collision.crash_time = *proxy.crash_time;
    collision.borough = *proxy.borough;
    collision.zip_code = *proxy.zip_code;
    collision.latitude = *proxy.latitude;
    collision.longitude = *proxy.longitude;
    collision.location = *proxy.location;
    collision.on_street_name = *proxy.on_street_name;
    collision.cross_street_name = *proxy.cross_street_name;
    collision.off_street_name = *proxy.off_street_name;
    collision.number_of_persons_injured = *proxy.number_of_persons_injured;
    collision.number_of_persons_killed = *proxy.number_of_persons_killed;
    collision.number_of_pedestrians_injured = *proxy.number_of_pedestrians_injured;
    collision.number_of_pedestrians_killed = *proxy.number_of_pedestrians_killed;
    collision.number_of_cyclist_injured = *proxy.number_of_cyclist_injured;
    collision.number_of_cyclist_killed = *proxy.number_of_cyclist_killed;
    collision.number_of_motorist_injured = *proxy.number_of_motorist_injured;
    collision.number_of_motorist_killed = *proxy.number_of_motorist_killed;
    collision.contributing_factor_vehicle_1 = *proxy.contributing_factor_vehicle_1;
    collision.contributing_factor_vehicle_2 = *proxy.contributing_factor_vehicle_2;
    collision.contributing_factor_vehicle_3 = *proxy.contributing_factor_vehicle_3;
    collision.contributing_factor_vehicle_4 = *proxy.contributing_factor_vehicle_4;
    collision.contributing_factor_vehicle_5 = *proxy.contributing_factor_vehicle_5;
    collision.collision_id = *proxy.collision_id;
    collision.vehicle_type_code_1 = *proxy.vehicle_type_code_1;
    collision.vehicle_type_code_2 = *proxy.vehicle_type_code_2;
    collision.vehicle_type_code_3 = *proxy.vehicle_type_code_3;
    collision.vehicle_type_code_4 = *proxy.vehicle_type_code_4;
    collision.vehicle_type_code_5 = *proxy.vehicle_type_code_5;
    return collision;
}
