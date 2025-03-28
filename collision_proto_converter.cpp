#include "collision_proto_converter.hpp"

#include <iostream>

QueryResponse CollisionProtoConverter::deserialize(const collision_proto::QueryResponse& response) {
    std::vector<Collision> collisions{};

    for (int i = 0; i < response.collision_size(); ++i) {
        const collision_proto::Collision& collision_proto = response.collision(i);
        Collision collision{};

        if (collision_proto.has_crash_date()) {
            collision.crash_date = collision_parser_converters::convert_year_month_day_date(collision_proto.crash_date());
        }
        if (collision_proto.has_crash_time()) {
            collision.crash_time = collision_parser_converters::convert_hour_minute_time(collision_proto.crash_time());
        }
        if (collision_proto.has_borough()) {
            collision.borough = CollisionString(collision_proto.borough().c_str());
        }
        if (collision_proto.has_zip_code()) {
            collision.zip_code = collision_proto.zip_code();
        }
        if (collision_proto.has_latitude()) {
            collision.latitude = collision_proto.latitude();
        }
        if (collision_proto.has_longitude()) {
            collision.longitude = collision_proto.longitude();
        }
        if (collision_proto.has_location()) {
            collision.location = CollisionString(collision_proto.location().c_str());
        }
        if (collision_proto.has_on_street_name()) {
            collision.on_street_name = CollisionString(collision_proto.on_street_name().c_str());
        }
        if (collision_proto.has_cross_street_name()) {
            collision.cross_street_name = CollisionString(collision_proto.cross_street_name().c_str());
        }
        if (collision_proto.has_off_street_name()) {
            collision.off_street_name = CollisionString(collision_proto.off_street_name().c_str());
        }
        if (collision_proto.has_number_of_persons_injured()) {
            collision.number_of_persons_injured = collision_proto.number_of_persons_injured();
        }
        if (collision_proto.has_number_of_persons_killed()) {
            collision.number_of_persons_killed = collision_proto.number_of_persons_killed();
        }
        if (collision_proto.has_number_of_pedestrians_injured()) {
            collision.number_of_pedestrians_injured = collision_proto.number_of_pedestrians_injured();
        }
        if (collision_proto.has_number_of_pedestrians_killed()) {
            collision.number_of_pedestrians_killed = collision_proto.number_of_pedestrians_killed();
        }
        if (collision_proto.has_number_of_cyclist_injured()) {
            collision.number_of_cyclist_injured = collision_proto.number_of_cyclist_injured();
        }
        if (collision_proto.has_number_of_cyclist_killed()) {
            collision.number_of_cyclist_killed = collision_proto.number_of_cyclist_killed();
        }
        if (collision_proto.has_number_of_motorist_injured()) {
            collision.number_of_motorist_injured = collision_proto.number_of_motorist_injured();
        }
        if (collision_proto.has_number_of_motorist_killed()) {
            collision.number_of_motorist_killed = collision_proto.number_of_motorist_killed();
        }
        if (collision_proto.has_contributing_factor_vehicle_1()) {
            collision.contributing_factor_vehicle_1 = CollisionString(collision_proto.contributing_factor_vehicle_1().c_str());
        }
        if (collision_proto.has_contributing_factor_vehicle_2()) {
            collision.contributing_factor_vehicle_2 = CollisionString(collision_proto.contributing_factor_vehicle_2().c_str());
        }
        if (collision_proto.has_contributing_factor_vehicle_3()) {
            collision.contributing_factor_vehicle_3 = CollisionString(collision_proto.contributing_factor_vehicle_3().c_str());
        }
        if (collision_proto.has_contributing_factor_vehicle_4()) {
            collision.contributing_factor_vehicle_4 = CollisionString(collision_proto.contributing_factor_vehicle_4().c_str());
        }
        if (collision_proto.has_contributing_factor_vehicle_5()) {
            collision.contributing_factor_vehicle_5 = CollisionString(collision_proto.contributing_factor_vehicle_5().c_str());
        }
        if (collision_proto.has_collision_id()) {
            collision.collision_id = collision_proto.collision_id();
        }
        if (collision_proto.has_vehicle_type_code_1()) {
            collision.vehicle_type_code_1 = CollisionString(collision_proto.vehicle_type_code_1().c_str());
        }
        if (collision_proto.has_vehicle_type_code_2()) {
            collision.vehicle_type_code_2 = CollisionString(collision_proto.vehicle_type_code_2().c_str());
        }
        if (collision_proto.has_vehicle_type_code_3()) {
            collision.vehicle_type_code_3 = CollisionString(collision_proto.vehicle_type_code_3().c_str());
        }
        if (collision_proto.has_vehicle_type_code_4()) {
            collision.vehicle_type_code_4 = CollisionString(collision_proto.vehicle_type_code_4().c_str());
        }
        if (collision_proto.has_vehicle_type_code_5()) {
            collision.vehicle_type_code_5 = CollisionString(collision_proto.vehicle_type_code_5().c_str());
        }
        collisions.push_back(collision);
    }

    std::vector<std::uint32_t> requested_by;
    for (int i = 0; i < response.requested_by_size(); ++i) {
        requested_by.push_back(response.requested_by(i));
    }

    QueryResponse query_response = {
        .id = response.id(),
        .requested_by = requested_by,
        .results_from = response.results_from(),
        .collisions = collisions,
    };

    return query_response;
}

collision_proto::QueryResponse CollisionProtoConverter::serialize(const QueryResponse& query_response) {
    collision_proto::QueryResponse proto_query_response;

    serialize(query_response, proto_query_response);

    return proto_query_response;
}

void CollisionProtoConverter::serialize(const QueryResponse& query_response, collision_proto::QueryResponse& proto_query_response) {
    proto_query_response.set_id(query_response.id);
    proto_query_response.set_results_from(query_response.results_from);

    for (const uint32_t req_by : query_response.requested_by) {
        proto_query_response.add_requested_by(req_by);
    }

    for (const Collision& collision : query_response.collisions) {
        collision_proto::Collision* proto_collision = proto_query_response.add_collision();

        if (collision.crash_date.has_value()) {
            std::chrono::year_month_day ymd = collision.crash_date.value();
            proto_collision->set_crash_date(std::format("{:02}/{:02}/{:04}", (unsigned)ymd.month(), (unsigned)ymd.day(), (int)ymd.year()));
        }
        if (collision.crash_time.has_value()) {
            std::chrono::hh_mm_ss<std::chrono::minutes> time = collision.crash_time.value();
            proto_collision->set_crash_time(std::format("{:02}:{:02}", (int)time.hours().count(), (int)time.minutes().count()));
        }
        if (collision.borough.has_value()) {
            proto_collision->set_borough(std::string_view(collision.borough.value().data, collision.borough.value().length));
        }
        if (collision.zip_code.has_value()) {
            proto_collision->set_zip_code(collision.zip_code.value());
        }
        if (collision.latitude.has_value()) {
            proto_collision->set_latitude(collision.latitude.value());
        }
        if (collision.longitude.has_value()) {
            proto_collision->set_longitude(collision.longitude.value());
        }
        if (collision.location.has_value()) {
            proto_collision->set_location(std::string_view(collision.location.value().data, collision.location.value().length));
        }
        if (collision.on_street_name.has_value()) {
            proto_collision->set_on_street_name(std::string_view(collision.on_street_name.value().data, collision.on_street_name.value().length));
        }
        if (collision.cross_street_name.has_value()) {
            proto_collision->set_cross_street_name(std::string_view(collision.cross_street_name.value().data, collision.cross_street_name.value().length));
        }
        if (collision.off_street_name.has_value()) {
            proto_collision->set_off_street_name(std::string_view(collision.off_street_name.value().data, collision.off_street_name.value().length));
        }
        if (collision.number_of_persons_injured.has_value()) {
            proto_collision->set_number_of_persons_injured(collision.number_of_persons_injured.value());
        }
        if (collision.number_of_persons_killed.has_value()) {
            proto_collision->set_number_of_persons_killed(collision.number_of_persons_killed.value());
        }
        if (collision.number_of_pedestrians_injured.has_value()) {
            proto_collision->set_number_of_pedestrians_injured(collision.number_of_pedestrians_injured.value());
        }
        if (collision.number_of_pedestrians_killed.has_value()) {
            proto_collision->set_number_of_pedestrians_killed(collision.number_of_pedestrians_killed.value());
        }
        if (collision.number_of_cyclist_injured.has_value()) {
            proto_collision->set_number_of_cyclist_injured(collision.number_of_cyclist_injured.value());
        }
        if (collision.number_of_cyclist_killed.has_value()) {
            proto_collision->set_number_of_cyclist_killed(collision.number_of_cyclist_killed.value());
        }
        if (collision.number_of_motorist_injured.has_value()) {
            proto_collision->set_number_of_motorist_injured(collision.number_of_motorist_injured.value());
        }
        if (collision.number_of_motorist_killed.has_value()) {
            proto_collision->set_number_of_motorist_killed(collision.number_of_motorist_killed.value());
        }
        if (collision.contributing_factor_vehicle_1.has_value()) {
            proto_collision->set_contributing_factor_vehicle_1(std::string_view(collision.contributing_factor_vehicle_1.value().data,
                                                                                collision.contributing_factor_vehicle_1.value().length));
        }
        if (collision.contributing_factor_vehicle_2.has_value()) {
            proto_collision->set_contributing_factor_vehicle_2(std::string_view(collision.contributing_factor_vehicle_2.value().data,
                                                                                collision.contributing_factor_vehicle_2.value().length));
        }
        if (collision.contributing_factor_vehicle_3.has_value()) {
            proto_collision->set_contributing_factor_vehicle_3(std::string_view(collision.contributing_factor_vehicle_3.value().data,
                                                                                collision.contributing_factor_vehicle_3.value().length));
        }
        if (collision.contributing_factor_vehicle_4.has_value()) {
            proto_collision->set_contributing_factor_vehicle_4(std::string_view(collision.contributing_factor_vehicle_4.value().data,
                                                                                collision.contributing_factor_vehicle_4.value().length));
        }
        if (collision.contributing_factor_vehicle_5.has_value()) {
            proto_collision->set_contributing_factor_vehicle_5(std::string_view(collision.contributing_factor_vehicle_5.value().data,
                                                                                collision.contributing_factor_vehicle_5.value().length));
        }
        if (collision.collision_id.has_value()) {
            proto_collision->set_collision_id(collision.collision_id.value());
        }
        if (collision.vehicle_type_code_1.has_value()) {
            proto_collision->set_vehicle_type_code_1(std::string_view(collision.vehicle_type_code_1.value().data,
                                                                      collision.vehicle_type_code_1.value().length));
        }
        if (collision.vehicle_type_code_2.has_value()) {
            proto_collision->set_vehicle_type_code_2(std::string_view(collision.vehicle_type_code_2.value().data,
                                                                      collision.vehicle_type_code_2.value().length));
        }
        if (collision.vehicle_type_code_3.has_value()) {
            proto_collision->set_vehicle_type_code_3(std::string_view(collision.vehicle_type_code_3.value().data,
                                                                      collision.vehicle_type_code_3.value().length));
        }
        if (collision.vehicle_type_code_4.has_value()) {
            proto_collision->set_vehicle_type_code_4(std::string_view(collision.vehicle_type_code_4.value().data,
                                                                      collision.vehicle_type_code_4.value().length));
        }
        if (collision.vehicle_type_code_5.has_value()) {
            proto_collision->set_vehicle_type_code_5(std::string_view(collision.vehicle_type_code_5.value().data,
                                                                      collision.vehicle_type_code_5.value().length));
        }
    }
}
