#pragma once

#include "collision_field_enum.hpp"

#include <cassert>
#include <chrono>
#include <string>
#include <string_view>

using Value = std::variant<
    float,
    size_t,
    std::string,
    std::chrono::year_month_day,
    std::chrono::hh_mm_ss<std::chrono::minutes>,
    std::uint8_t,
    std::uint32_t>;

enum class QueryType { HAS_VALUE, EQUALS, LESS_THAN, GREATER_THAN, CONTAINS };
enum class Qualifier { NONE, NOT, CASE_INSENSITIVE };

class FieldQuery {
private:
    FieldQuery(const CollisionField& name, const QueryType& type, const Value& value, bool invert_match, bool case_insensitive)
      : name_{name},
        type_{type},
        value_{value},
        invert_match_{invert_match},
        case_insensitive_{case_insensitive} {}

    const CollisionField name_;
    const QueryType type_;
    const Value value_;
    const bool invert_match_;
    const bool case_insensitive_;

public:
    friend class Query;
    friend class QueryProtoConverter;

    const CollisionField& get_name() const;
    const QueryType& get_type() const;
    const Value& get_value() const;
    const bool invert_match() const;
    const bool case_insensitive() const;
};

class Query {
private:
    Query(FieldQuery&& query)
      : queries{query}
      {}

    std::vector<FieldQuery> queries;

    static FieldQuery create_field_query(const CollisionField& name,
                                         const Qualifier& not_qualifier,
                                         const QueryType& type,
                                         const Value value,
                                         const Qualifier& case_insensitive_qualifier);

public:
    const std::vector<FieldQuery>& get() const;

    Query& add(const Query& query);
    Query& add(const CollisionField& name, const QueryType& type, const Value value);
    Query& add(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value);
    Query& add(const CollisionField& name, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier);
    Query& add(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier);

    static Query create(const CollisionField& name, const QueryType& type, const Value value);
    static Query create(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value);
    static Query create(const CollisionField& name, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier);
    static Query create(const CollisionField& name, const Qualifier& not_qualifier, const QueryType& type, const Value value, const Qualifier& case_insensitive_qualifier);

private:
    Query& add(const FieldQuery&& field_query);
};
