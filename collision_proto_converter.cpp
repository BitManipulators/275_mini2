#include "collision_proto_converter.hpp"

#include <iostream>

Collision CollisionProtoConverter::deserialize(const collision_proto::Collision* collision_proto) {
    Collision collision{};

    // TODO

    return collision;
}

collision_proto::Collision CollisionProtoConverter::serialize(const Collision& collision) {
    collision_proto::Collision proto;

    // TODO

    return proto;
}

void CollisionProtoConverter::serialize(const CollisionProxy* data_struct, collision_proto::Collision* proto_data) {

   // Convert crash date (assuming it's in the form std::chrono::year_month_day)

            /*
            if (data_struct->crash_date != std::chrono::year_month_day{}) {  // Check if the date is set
                std::ostringstream date_stream;
                date_stream << std::chrono::year(data_struct->crash_date) << "-"
                            << std::chrono::month(data_struct->crash_date) << "-"
                            << std::chrono::day(data_struct->crash_date);
                proto_data->set_crash_date(date_stream.str());
            }

            // Convert crash time (assuming it's in std::chrono::hh_mm_ss)
            if (data_struct->crash_time != std::chrono::hh_mm_ss<std::chrono::minutes>{}) {  // Check if the time is set
                std::ostringstream time_stream;
                time_stream << std::chrono::hours(data_struct->crash_time) << ":"
                            << std::chrono::minutes(data_struct->crash_time) << ":"
                            << std::chrono::seconds(data_struct->crash_time);
                proto_data->set_crash_time(time_stream.str());
            }*/

            // Convert optional fields using has_value() for std::optional

            if (data_struct->borough && data_struct->borough->has_value()) {
                proto_data->set_borough(data_struct->borough->value());
            }

            if (data_struct->zip_code && data_struct->zip_code->has_value()) {
                proto_data->set_zip_code(data_struct->zip_code->value());
            }

            if (data_struct->latitude && data_struct->latitude->has_value()) {
                proto_data->set_latitude(data_struct->latitude->value());
            }

            if (data_struct->longitude && data_struct->longitude->has_value()) {
                proto_data->set_longitude(data_struct->longitude->value());
            }

            if (data_struct->location && data_struct->location->has_value()) {
                proto_data->set_location(data_struct->location->value());
            }

            if (data_struct->on_street_name && data_struct->on_street_name->has_value()) {
                proto_data->set_on_street_name(data_struct->on_street_name->value());
            }

            if (data_struct->cross_street_name && data_struct->cross_street_name->has_value()) {
                proto_data->set_cross_street_name(data_struct->cross_street_name->value());
            }

            if (data_struct->off_street_name && data_struct->off_street_name->has_value()) {
                proto_data->set_off_street_name(data_struct->off_street_name->value());
            }

            if (data_struct->number_of_persons_injured && data_struct->number_of_persons_injured->has_value()) {
                proto_data->set_number_of_persons_injured(data_struct->number_of_persons_injured->value());
            }

            if (data_struct->number_of_persons_killed && data_struct->number_of_persons_killed->has_value()) {
                proto_data->set_number_of_persons_killed(data_struct->number_of_persons_killed->value());
            }

            if (data_struct->number_of_pedestrians_injured && data_struct->number_of_pedestrians_injured->has_value()) {
                proto_data->set_number_of_pedestrians_injured(data_struct->number_of_pedestrians_injured->value());
            }

            if (data_struct->number_of_pedestrians_killed && data_struct->number_of_pedestrians_killed->has_value()) {
                proto_data->set_number_of_pedestrians_killed(data_struct->number_of_pedestrians_killed->value());
            }

            if (data_struct->number_of_cyclist_injured && data_struct->number_of_cyclist_injured->has_value()) {
                proto_data->set_number_of_cyclist_injured(data_struct->number_of_cyclist_injured->value());
            }

            if (data_struct->number_of_cyclist_killed && data_struct->number_of_cyclist_killed->has_value()) {
                proto_data->set_number_of_cyclist_killed(data_struct->number_of_cyclist_killed->value());
            }

            if (data_struct->number_of_motorist_injured && data_struct->number_of_motorist_injured->has_value()) {
                proto_data->set_number_of_motorist_injured(data_struct->number_of_motorist_injured->value());
            }

            if (data_struct->number_of_motorist_killed && data_struct->number_of_motorist_killed->has_value()) {
                proto_data->set_number_of_motorist_killed(data_struct->number_of_motorist_killed->value());
            }

            if (data_struct->contributing_factor_vehicle_1 && data_struct->contributing_factor_vehicle_1->has_value()) {
                proto_data->set_contributing_factor_vehicle_1(data_struct->contributing_factor_vehicle_1->value());
            }

            if (data_struct->contributing_factor_vehicle_2 && data_struct->contributing_factor_vehicle_2->has_value()) {
                proto_data->set_contributing_factor_vehicle_2(data_struct->contributing_factor_vehicle_2->value());
            }

            if (data_struct->contributing_factor_vehicle_3 && data_struct->contributing_factor_vehicle_3->has_value()) {
                proto_data->set_contributing_factor_vehicle_3(data_struct->contributing_factor_vehicle_3->value());
            }

            if (data_struct->contributing_factor_vehicle_4 && data_struct->contributing_factor_vehicle_4->has_value()) {
                proto_data->set_contributing_factor_vehicle_4(data_struct->contributing_factor_vehicle_4->value());
            }

            if (data_struct->contributing_factor_vehicle_5 && data_struct->contributing_factor_vehicle_5->has_value()) {
                proto_data->set_contributing_factor_vehicle_5(data_struct->contributing_factor_vehicle_5->value());
            }

            if (data_struct->collision_id && data_struct->collision_id->has_value()) {
                proto_data->set_collision_id(data_struct->collision_id->value());
            }

            if (data_struct->vehicle_type_code_1 && data_struct->vehicle_type_code_1->has_value()) {
                proto_data->set_vehicle_type_code_1(data_struct->vehicle_type_code_1->value());
            }

            if (data_struct->vehicle_type_code_2 && data_struct->vehicle_type_code_2->has_value()) {
                proto_data->set_vehicle_type_code_2(data_struct->vehicle_type_code_2->value());
            }

            if (data_struct->vehicle_type_code_3 && data_struct->vehicle_type_code_3->has_value()) {
                proto_data->set_vehicle_type_code_3(data_struct->vehicle_type_code_3->value());
            }

            if (data_struct->vehicle_type_code_4 && data_struct->vehicle_type_code_4->has_value()) {
                proto_data->set_vehicle_type_code_4(data_struct->vehicle_type_code_4->value());
            }

            if (data_struct->vehicle_type_code_5 && data_struct->vehicle_type_code_5->has_value()) {
                proto_data->set_vehicle_type_code_5(data_struct->vehicle_type_code_5->value());
            }
}
