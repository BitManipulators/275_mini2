#include "query_proto_converter.hpp"

#include <iostream>

collision_proto::QueryFields to_proto_query_field(CollisionField field) {
    switch (field) {
        case CollisionField::CRASH_DATE: return collision_proto::QueryFields::CRASH_DATE;
        case CollisionField::CRASH_TIME: return collision_proto::QueryFields::CRASH_TIME;
        case CollisionField::BOROUGH: return collision_proto::QueryFields::BOROUGH;
        case CollisionField::ZIP_CODE: return collision_proto::QueryFields::ZIP_CODE;
        case CollisionField::LATITUDE: return collision_proto::QueryFields::LATITUDE;
        case CollisionField::LONGITUDE: return collision_proto::QueryFields::LONGITUDE;
        case CollisionField::LOCATION: return collision_proto::QueryFields::LOCATION;
        case CollisionField::ON_STREET_NAME: return collision_proto::QueryFields::ON_STREET_NAME;
        case CollisionField::CROSS_STREET_NAME: return collision_proto::QueryFields::CROSS_STREET_NAME;
        case CollisionField::OFF_STREET_NAME: return collision_proto::QueryFields::OFF_STREET_NAME;
        case CollisionField::NUMBER_OF_PERSONS_INJURED: return collision_proto::QueryFields::NUMBER_OF_PERSONS_INJURED;
        case CollisionField::NUMBER_OF_PERSONS_KILLED: return collision_proto::QueryFields::NUMBER_OF_PERSONS_KILLED;
        case CollisionField::NUMBER_OF_PEDESTRIANS_INJURED: return collision_proto::QueryFields::NUMBER_OF_PEDESTRIANS_INJURED;
        case CollisionField::NUMBER_OF_PEDESTRIANS_KILLED: return collision_proto::QueryFields::NUMBER_OF_PEDESTRIANS_KILLED;
        case CollisionField::NUMBER_OF_CYCLIST_INJURED: return collision_proto::QueryFields::NUMBER_OF_CYCLIST_INJURED;
        case CollisionField::NUMBER_OF_CYCLIST_KILLED: return collision_proto::QueryFields::NUMBER_OF_CYCLIST_KILLED;
        case CollisionField::NUMBER_OF_MOTORIST_INJURED: return collision_proto::QueryFields::NUMBER_OF_MOTORIST_INJURED;
        case CollisionField::NUMBER_OF_MOTORIST_KILLED: return collision_proto::QueryFields::NUMBER_OF_MOTORIST_KILLED;
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1: return collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_1;
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2: return collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_2;
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3: return collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_3;
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4: return collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_4;
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5: return collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_5;
        case CollisionField::COLLISION_ID: return collision_proto::QueryFields::COLLISION_ID;
        case CollisionField::VEHICLE_TYPE_CODE_1: return collision_proto::QueryFields::VEHICLE_TYPE_CODE_1;
        case CollisionField::VEHICLE_TYPE_CODE_2: return collision_proto::QueryFields::VEHICLE_TYPE_CODE_2;
        case CollisionField::VEHICLE_TYPE_CODE_3: return collision_proto::QueryFields::VEHICLE_TYPE_CODE_3;
        case CollisionField::VEHICLE_TYPE_CODE_4: return collision_proto::QueryFields::VEHICLE_TYPE_CODE_4;
        case CollisionField::VEHICLE_TYPE_CODE_5: return collision_proto::QueryFields::VEHICLE_TYPE_CODE_5;
        case CollisionField::UNDEFINED:
        default: throw std::invalid_argument("Unknown CollisionField value");
    }
}

CollisionField from_proto_query_field(collision_proto::QueryFields field) {
    switch (field) {
        case collision_proto::QueryFields::CRASH_DATE: return CollisionField::CRASH_DATE;
        case collision_proto::QueryFields::CRASH_TIME: return CollisionField::CRASH_TIME;
        case collision_proto::QueryFields::BOROUGH: return CollisionField::BOROUGH;
        case collision_proto::QueryFields::ZIP_CODE: return CollisionField::ZIP_CODE;
        case collision_proto::QueryFields::LATITUDE: return CollisionField::LATITUDE;
        case collision_proto::QueryFields::LONGITUDE: return CollisionField::LONGITUDE;
        case collision_proto::QueryFields::LOCATION: return CollisionField::LOCATION;
        case collision_proto::QueryFields::ON_STREET_NAME: return CollisionField::ON_STREET_NAME;
        case collision_proto::QueryFields::CROSS_STREET_NAME: return CollisionField::CROSS_STREET_NAME;
        case collision_proto::QueryFields::OFF_STREET_NAME: return CollisionField::OFF_STREET_NAME;
        case collision_proto::QueryFields::NUMBER_OF_PERSONS_INJURED: return CollisionField::NUMBER_OF_PERSONS_INJURED;
        case collision_proto::QueryFields::NUMBER_OF_PERSONS_KILLED: return CollisionField::NUMBER_OF_PERSONS_KILLED;
        case collision_proto::QueryFields::NUMBER_OF_PEDESTRIANS_INJURED: return CollisionField::NUMBER_OF_PEDESTRIANS_INJURED;
        case collision_proto::QueryFields::NUMBER_OF_PEDESTRIANS_KILLED: return CollisionField::NUMBER_OF_PEDESTRIANS_KILLED;
        case collision_proto::QueryFields::NUMBER_OF_CYCLIST_INJURED: return CollisionField::NUMBER_OF_CYCLIST_INJURED;
        case collision_proto::QueryFields::NUMBER_OF_CYCLIST_KILLED: return CollisionField::NUMBER_OF_CYCLIST_KILLED;
        case collision_proto::QueryFields::NUMBER_OF_MOTORIST_INJURED: return CollisionField::NUMBER_OF_MOTORIST_INJURED;
        case collision_proto::QueryFields::NUMBER_OF_MOTORIST_KILLED: return CollisionField::NUMBER_OF_MOTORIST_KILLED;
        case collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_1: return CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1;
        case collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_2: return CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2;
        case collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_3: return CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3;
        case collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_4: return CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4;
        case collision_proto::QueryFields::CONTRIBUTING_FACTOR_VEHICLE_5: return CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5;
        case collision_proto::QueryFields::COLLISION_ID: return CollisionField::COLLISION_ID;
        case collision_proto::QueryFields::VEHICLE_TYPE_CODE_1: return CollisionField::VEHICLE_TYPE_CODE_1;
        case collision_proto::QueryFields::VEHICLE_TYPE_CODE_2: return CollisionField::VEHICLE_TYPE_CODE_2;
        case collision_proto::QueryFields::VEHICLE_TYPE_CODE_3: return CollisionField::VEHICLE_TYPE_CODE_3;
        case collision_proto::QueryFields::VEHICLE_TYPE_CODE_4: return CollisionField::VEHICLE_TYPE_CODE_4;
        case collision_proto::QueryFields::VEHICLE_TYPE_CODE_5: return CollisionField::VEHICLE_TYPE_CODE_5;
        default: throw std::invalid_argument("Unknown QueryFields value");
    }
}

Value from_proto_query_value(const collision_proto::QueryCondition& proto_query_condition, const CollisionField collision_field) {
    FieldValueType field_value_type = field_to_value_type(collision_field);
    switch(field_value_type) {
        case FieldValueType::UINT8_T:
            return static_cast<std::uint8_t>(proto_query_condition.uint8_data());
        case FieldValueType::UINT32_T:
            return proto_query_condition.uint32_data();
        case FieldValueType::SIZE_T:
            return proto_query_condition.uint64_data();
        case FieldValueType::FLOAT:
            return proto_query_condition.float_data();
        case FieldValueType::STRING:
            return proto_query_condition.string_data();
        case FieldValueType::DATE:
        case FieldValueType::TIME:
        default:
            throw std::invalid_argument("Currently unsupported field types");
    }
}

QueryType from_proto_query_type(const collision_proto::QueryType& proto_query_type) {
    switch (proto_query_type) {
        case collision_proto::QueryType::HAS_VALUE:
            return QueryType::HAS_VALUE;
        case collision_proto::QueryType::EQUALS:
            return QueryType::EQUALS;
        case collision_proto::QueryType::LESS_THAN:
            return QueryType::LESS_THAN;
        case collision_proto::QueryType::GREATER_THAN:
            return QueryType::GREATER_THAN;
        case collision_proto::QueryType::CONTAINS:
            return QueryType::CONTAINS;
        default:
            throw std::invalid_argument("Unknown query type");
    }
}

collision_proto::QueryType to_proto_query_type(const QueryType& query_type) {
    switch (query_type) {
        case QueryType::HAS_VALUE:
            return collision_proto::QueryType::HAS_VALUE;
        case QueryType::EQUALS:
            return collision_proto::QueryType::EQUALS;
        case QueryType::LESS_THAN:
            return collision_proto::QueryType::LESS_THAN;
        case QueryType::GREATER_THAN:
            return collision_proto::QueryType::GREATER_THAN;
        case QueryType::CONTAINS:
            return collision_proto::QueryType::CONTAINS;
        default:
            throw std::invalid_argument("Unknown query type");
    }
}

Query QueryProtoConverter::deserialize(const collision_proto::QueryRequest* request) {
    std::vector<Query> queries;
    std::cout << "Request query size - " << request->queries().size() << std::endl;

    for (const auto& proto_query : request->queries()) {
        CollisionField field = from_proto_query_field(proto_query.field());
        Value value = from_proto_query_value(proto_query, field);
        QueryType type = from_proto_query_type(proto_query.type());

        Qualifier not_ = Qualifier::NONE;
        if (proto_query.not_()) {
            not_ = Qualifier::NOT;
        }

        Qualifier case_insensitive = Qualifier::NONE;
        if (proto_query.case_insensitive()) {
            case_insensitive = Qualifier::CASE_INSENSITIVE;
        }

        queries.push_back(Query::create(field, not_, type, value, case_insensitive));
    }

    std::cout << "vector query size - " << queries.size() << std::endl;

    Query& query = queries.at(0);
    for (auto it = queries.begin() + 1; it < queries.end(); ++it) {
        query.add(*it);
    }
    return query;
}

void serialize_proto_data(collision_proto::QueryCondition* condition, const FieldQuery& field_query) {
    std::visit([&condition](auto&& val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, float>) {
            condition->set_float_data(val);
        } else if constexpr (std::is_same_v<T, std::uint8_t>) {
            condition->set_uint8_data(val);
        } else if constexpr (std::is_same_v<T, std::uint32_t>) {
            condition->set_uint32_data(val);
        } else if constexpr (std::is_same_v<T, std::size_t>) {
            condition->set_uint64_data(val);
        } else if constexpr (std::is_same_v<T, std::string>) {
            condition->set_string_data(val);
        } else if constexpr (std::is_same_v<T, std::chrono::year_month_day>) {
            throw std::invalid_argument("Date not support yet!");
        } else if constexpr (std::is_same_v<T, std::chrono::hh_mm_ss<std::chrono::minutes>>) {
            throw std::invalid_argument("Time not supported yet!");
        }
    }, field_query.get_value());
}

collision_proto::QueryRequest QueryProtoConverter::serialize(const Query& query) {
    collision_proto::QueryRequest request;

    for (auto& field_query : query.get()) {
        collision_proto::QueryFields field = to_proto_query_field(field_query.get_name());
        collision_proto::QueryType type = to_proto_query_type(field_query.get_type());

        collision_proto::QueryCondition* condition  = request.add_queries();
        condition->set_field(field);
        condition->set_type(type);

        serialize_proto_data(condition, field_query);

        if (field_query.invert_match()) {
            condition->set_not_(true);
        }

        if (field_query.case_insensitive()) {
            condition->set_case_insensitive(true);
        }
    }

    return request;
}
