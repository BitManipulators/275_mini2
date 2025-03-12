#include "query.hpp"

#include "collision_field_enum.hpp"

#include <chrono>
#include <stdexcept>
#include <string>

const CollisionField& FieldQuery::get_name() const {
    return name_;
}

const QueryType& FieldQuery::get_type() const {
    return type_;
}

const Value& FieldQuery::get_value() const {
    return value_;
}

const bool FieldQuery::invert_match() const {
    return invert_match_;
}

const bool FieldQuery::case_insensitive() const {
    return case_insensitive_;
}

FieldQuery Query::create_field_query(const CollisionField& name,
                                     const Qualifier& not_qualifier,
                                     const QueryType& type,
                                     const Value value,
                                     const Qualifier& case_insensitive_qualifier) {
    std::visit([&name](auto&& val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, float>) {
            if (name != CollisionField::LATITUDE && name != CollisionField::LONGITUDE) {
                throw std::invalid_argument("Invalid field_name provided for float!");
            }
        } else if constexpr (std::is_same_v<T, std::uint8_t>) {
            if (name != CollisionField::NUMBER_OF_PERSONS_INJURED && name != CollisionField::NUMBER_OF_PERSONS_KILLED &&
                name != CollisionField::NUMBER_OF_PEDESTRIANS_INJURED && name != CollisionField::NUMBER_OF_PEDESTRIANS_KILLED &&
                name != CollisionField::NUMBER_OF_CYCLIST_INJURED && name != CollisionField::NUMBER_OF_CYCLIST_KILLED &&
                name != CollisionField::NUMBER_OF_MOTORIST_INJURED && name != CollisionField::NUMBER_OF_MOTORIST_KILLED) {
                throw std::invalid_argument("Invalid field_name provided for std::uint8_t!");
            }
        } else if constexpr (std::is_same_v<T, std::uint32_t>) {
            if (name != CollisionField::ZIP_CODE) {
                throw std::invalid_argument("Invalid field_name provided for std::uint32_t!");
            }
        } else if constexpr (std::is_same_v<T, std::size_t>) {
            if (name != CollisionField::COLLISION_ID) {
                throw std::invalid_argument("Invalid field_name provided for std::size_t!");
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (name != CollisionField::BOROUGH && name != CollisionField::LOCATION && name != CollisionField::ON_STREET_NAME &&
                name != CollisionField::CROSS_STREET_NAME && name != CollisionField::OFF_STREET_NAME &&
                name != CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1 && name != CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2 &&
                name != CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3 && name != CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4 &&
                name != CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5 && name != CollisionField::VEHICLE_TYPE_CODE_1 &&
                name != CollisionField::VEHICLE_TYPE_CODE_2 && name != CollisionField::VEHICLE_TYPE_CODE_3 &&
                name != CollisionField::VEHICLE_TYPE_CODE_4 && name != CollisionField::VEHICLE_TYPE_CODE_5) {
                throw std::invalid_argument("Invalid field_name provided for std::string!");
            }
        } else if constexpr (std::is_same_v<T, std::chrono::year_month_day>) {
            if (name != CollisionField::CRASH_DATE) {
                throw std::invalid_argument("Invalid field_name provided for std::chrono::year_month_day!");
            }
        } else if constexpr (std::is_same_v<T, std::chrono::hh_mm_ss<std::chrono::minutes>>) {
            if (name != CollisionField::CRASH_TIME) {
                throw std::invalid_argument("Invalid field_name provided for std::chrono::hh_mm_ss!");
            }
        }
    }, value);

    return FieldQuery(name,
                      type,
                      value,
                      not_qualifier == Qualifier::NOT,
                      case_insensitive_qualifier == Qualifier::CASE_INSENSITIVE);
}

const std::vector<FieldQuery>& Query::get() const {
    return queries;
}

Query& Query::add(const CollisionField& name, const QueryType& type, const Value value) {
    return add(name, Qualifier::NONE, type, value, Qualifier::NONE);
}

Query& Query::add(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value) {
    return add(name, not_qualifier, type, value, Qualifier::NONE);
}

Query& Query::add(const CollisionField& name, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier) {
    return add(name, Qualifier::NONE, type, value, case_insensitive_qualifier);
}

Query& Query::add(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier) {
    return add(create_field_query(name,
                                  not_qualifier,
                                  type,
                                  value,
                                  case_insensitive_qualifier));
}

Query& Query::add(const Query& query) {
    for (const FieldQuery& field_query : query.queries) {
        add(std::move(field_query));
    }
    return *this;
}

Query& Query::add(const FieldQuery&& field_query) {
    queries.push_back(field_query);
    return *this;
}

Query Query::create(const CollisionField& name, const QueryType& type, const Value value) {
    return create(name, Qualifier::NONE, type, value, Qualifier::NONE);
}

Query Query::create(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value) {
    return create(name, not_qualifier, type, value, Qualifier::NONE);
}

Query Query::create(const CollisionField& name, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier) {
    return create(name, Qualifier::NONE, type, value, case_insensitive_qualifier);
}

Query Query::create(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier) {
    return Query(create_field_query(name,
                                    not_qualifier,
                                    type,
                                    value,
                                    case_insensitive_qualifier));
}
