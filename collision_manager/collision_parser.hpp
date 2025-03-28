#pragma once

#include "collision.hpp"

#include <string>

namespace collision_parser_converters {

inline std::optional<std::chrono::year_month_day> convert_year_month_day_date(const std::string_view& field) {
    std::size_t first_slash = field.find('/');
    if (first_slash == std::string_view::npos) {
        std::cerr << "Error parsing date: " << field << std::endl;
        return {};
    }

    std::size_t second_slash = field.find('/', first_slash + 1);
    if (second_slash == std::string_view::npos) {
        std::cerr << "Error parsing date: " << field << std::endl;
        return {};
    }

    unsigned int month, day, year;

    auto month_result = std::from_chars(field.data(), field.data() + first_slash, month);
    auto day_result = std::from_chars(field.data() + first_slash + 1, field.data() + second_slash, day);
    auto year_result = std::from_chars(field.data() + second_slash + 1, field.data() + field.size() + 1, year);

    if (month_result.ec != std::errc() || day_result.ec != std::errc() || year_result.ec != std::errc()) {
        std::cerr << "Error parsing date: " << std::quoted(field) << std::endl;
        return {};
    }

    return std::chrono::year_month_day{std::chrono::year(year), std::chrono::month(month), std::chrono::day(day)};
}

inline std::optional<std::chrono::hh_mm_ss<std::chrono::minutes>> convert_hour_minute_time(const std::string_view& field) {
    std::size_t colon_index = field.find(':');
    if (colon_index == std::string_view::npos) {
        std::cerr << "Error parsing time: " << field << std::endl;
        return {};
    }

    unsigned int hour, minute;

    auto hour_result = std::from_chars(field.data(), field.data() + colon_index, hour);
    auto minute_result = std::from_chars(field.data() + colon_index + 1, field.data() + field.size(), minute);

    if (hour_result.ec != std::errc() || minute_result.ec != std::errc()) {
        std::cerr << "Error parsing time: " << std::quoted(field) << std::endl;
        return {};
    }

    return std::chrono::hh_mm_ss{std::chrono::hours(hour) + std::chrono::minutes(minute)};
}

}  // namespace collision_parser_converters

class CollisionParser {

public:
    CollisionParser(const std::string& filename);
    Collisions parse();
    Collisions parsePartition(int start_index, int end_index);
    int getTotalRecords();

private:
    std::string filename;
};
