#include "collision_parser.hpp"

#include "collision.hpp"
#include "collision_field_enum.hpp"

#include <chrono>
#include <iostream>
#include <format>
#include <fstream>
#include <omp.h>
#include <string>
#include <string_view>

bool contains_non_whitespace(const std::string_view& field) {
    for (char c : field) {
        if (!std::isspace(c)) {
            return true;
        }
    }
    return false;
}

std::optional<std::string> convert_string(const std::string_view& field) {
    return std::string(field);
}

std::optional<CollisionString> convert_fixed_string(const std::string_view& field) {
    return CollisionString(field);
}

template<typename T>
std::optional<T> convert_number(const std::string_view& field) {
    T number;

    auto number_result = std::from_chars(field.data(), field.data() + field.size(), number);

    if (number_result.ec != std::errc()) {
        std::cerr << "Error parsing number: " << std::quoted(field) << std::endl;
        return {};
    }

    return number;
}

void parseline(const std::string& line, Collisions& collisions) {

    bool is_inside_quote = false;
    std::size_t count = 0;
    std::size_t last_comma = 0;
    std::size_t next_comma = 0;
    std::size_t field_index = 0;

    Collision collision{};
    for (char c : line) {
        count++;

        if (c == '"') {
            is_inside_quote = !is_inside_quote;
        // Is this a comma?
        } else if (c == ',') {
            if (is_inside_quote) {
                continue;
            }

            next_comma = count - 1;

            // Is the field non-empty?
            if ((next_comma - last_comma) > 1) {
                std::string_view field = {line.data() + last_comma + (field_index > 0 ? 1 : 0), next_comma - last_comma - 1};

                if (contains_non_whitespace(field)) {
                    CollisionField collision_field = CollisionField::UNDEFINED;
                    if (field_index < static_cast<std::underlying_type_t<CollisionField>>(CollisionField::UNDEFINED)) {
                        collision_field = static_cast<CollisionField>(field_index);
                    }

                    switch(collision_field) {
                        case CollisionField::CRASH_DATE:
                            collision.crash_date = collision_parser_converters::convert_year_month_day_date(field);
                            break;
                        case CollisionField::CRASH_TIME:
                            collision.crash_time = collision_parser_converters::convert_hour_minute_time(field);
                            break;
                        case CollisionField::BOROUGH:
                            collision.borough = convert_fixed_string(field);
                            break;
                        case CollisionField::ZIP_CODE:
                            collision.zip_code = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::LATITUDE:
                            collision.latitude = convert_number<float>(field);
                            break;
                        case CollisionField::LONGITUDE:
                            collision.longitude = convert_number<float>(field);
                            break;
                        case CollisionField::LOCATION:
                            collision.location = convert_fixed_string(field);
                            break;
                        case CollisionField::ON_STREET_NAME:
                            collision.on_street_name = convert_fixed_string(field);
                            break;
                        case CollisionField::CROSS_STREET_NAME:
                            collision.cross_street_name = convert_fixed_string(field);
                            break;
                        case CollisionField::OFF_STREET_NAME:
                            collision.off_street_name = convert_fixed_string(field);
                            break;
                        case CollisionField::NUMBER_OF_PERSONS_INJURED:
                            collision.number_of_persons_injured = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_PERSONS_KILLED:
                            collision.number_of_persons_killed = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_PEDESTRIANS_INJURED:
                            collision.number_of_pedestrians_injured = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_PEDESTRIANS_KILLED:
                            collision.number_of_pedestrians_killed = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_CYCLIST_INJURED:
                            collision.number_of_cyclist_injured = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_CYCLIST_KILLED:
                            collision.number_of_cyclist_killed = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_MOTORIST_INJURED:
                            collision.number_of_motorist_injured = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::NUMBER_OF_MOTORIST_KILLED:
                            collision.number_of_motorist_killed = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1:
                            collision.contributing_factor_vehicle_1 = convert_fixed_string(field);
                            break;
                        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2:
                            collision.contributing_factor_vehicle_2 = convert_fixed_string(field);
                            break;
                        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3:
                            collision.contributing_factor_vehicle_3 = convert_fixed_string(field);
                            break;
                        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4:
                            collision.contributing_factor_vehicle_4 = convert_fixed_string(field);
                            break;
                        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5:
                            collision.contributing_factor_vehicle_5 = convert_fixed_string(field);
                            break;
                        case CollisionField::COLLISION_ID:
                            collision.collision_id = convert_number<std::size_t>(field);
                            break;
                        case CollisionField::VEHICLE_TYPE_CODE_1:
                            collision.vehicle_type_code_1 = convert_fixed_string(field);
                            break;
                        case CollisionField::VEHICLE_TYPE_CODE_2:
                            collision.vehicle_type_code_2 = convert_fixed_string(field);
                            break;
                        case CollisionField::VEHICLE_TYPE_CODE_3:
                            collision.vehicle_type_code_3 = convert_fixed_string(field);
                            break;
                        case CollisionField::VEHICLE_TYPE_CODE_4:
                            collision.vehicle_type_code_4 = convert_fixed_string(field);
                            break;
                        case CollisionField::VEHICLE_TYPE_CODE_5:
                            collision.vehicle_type_code_5 = convert_fixed_string(field);
                            break;
                        case CollisionField::UNDEFINED:
                        default:
                            std::cerr << "Unknown field_index: " << field_index << std::endl;
                    }
                }
            }

            last_comma = next_comma;
            field_index++;
        }
    }

    if (field_index != 28) {
        std::cerr << "Too few fields on csv line: " << line << std::endl;
        return;
    }

    collisions.add(collision);
}

CollisionParser::CollisionParser(const std::string& filename)
  : filename(filename) {}

Collisions CollisionParser::parse() {
    std::ifstream file{std::string(this->filename)};

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + this->filename);
    }

    std::string line;
    std::vector<std::string> lines;

    bool is_first_line = true;
    while (std::getline(file, line)) {
        if (is_first_line) {
            is_first_line = false;
            continue;
        }

        lines.push_back(line);
    }

    Collisions collisions{};

    unsigned long num_threads = omp_get_max_threads();
    std::vector<Collisions> thread_local_collisions{num_threads};

    #pragma omp parallel for schedule(static)
    for (const std::string& line : lines) {
        int thread_id = omp_get_thread_num();
        parseline(line, thread_local_collisions[thread_id]);
    }

    for (const auto& thread_collisions : thread_local_collisions) {
        collisions.combine(thread_collisions);
    }

    return collisions;
}

Collisions CollisionParser::parsePartition(int start_index, int end_index) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    }

    std::string line;
    std::vector<std::string> lines;
    bool is_first_line = true;

    // Read all lines; skip the header
    while (std::getline(file, line)) {
        if (is_first_line) {
            is_first_line = false;
            continue;
        }
        lines.push_back(line);
    }
    file.close();

    // Adjust indices if the file has fewer lines than expected.
    if (end_index > static_cast<int>(lines.size()))
        end_index = static_cast<int>(lines.size());

    Collisions collisions{};

    unsigned long num_threads = omp_get_max_threads();
    std::vector<Collisions> thread_local_collisions(num_threads);

    // Process only the lines that fall within [start_index, end_index)
    #pragma omp parallel for schedule(static)
    for (int i = start_index; i < end_index; i++) {
        const std::string& line = lines[i];
        int thread_id = omp_get_thread_num();
        parseline(line, thread_local_collisions[thread_id]);
    }

    // Combine results from all threads
    for (const auto& local_collisions : thread_local_collisions) {
        collisions.combine(local_collisions);
    }

    return collisions;
}

int CollisionParser::getTotalRecords() {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    }

    int totalRecords = 0;
    std::string line;
    while(std::getline(file, line)) {
        totalRecords++;
    }

    file.close();
    return totalRecords;
}
