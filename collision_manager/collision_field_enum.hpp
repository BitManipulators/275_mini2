#pragma once

#include <stdexcept>

enum class CollisionField {
    CRASH_DATE = 0,
    CRASH_TIME,
    BOROUGH,
    ZIP_CODE,
    LATITUDE,
    LONGITUDE,
    LOCATION,
    ON_STREET_NAME,
    CROSS_STREET_NAME,
    OFF_STREET_NAME,
    NUMBER_OF_PERSONS_INJURED,
    NUMBER_OF_PERSONS_KILLED,
    NUMBER_OF_PEDESTRIANS_INJURED,
    NUMBER_OF_PEDESTRIANS_KILLED,
    NUMBER_OF_CYCLIST_INJURED,
    NUMBER_OF_CYCLIST_KILLED,
    NUMBER_OF_MOTORIST_INJURED,
    NUMBER_OF_MOTORIST_KILLED,
    CONTRIBUTING_FACTOR_VEHICLE_1,
    CONTRIBUTING_FACTOR_VEHICLE_2,
    CONTRIBUTING_FACTOR_VEHICLE_3,
    CONTRIBUTING_FACTOR_VEHICLE_4,
    CONTRIBUTING_FACTOR_VEHICLE_5,
    COLLISION_ID,
    VEHICLE_TYPE_CODE_1,
    VEHICLE_TYPE_CODE_2,
    VEHICLE_TYPE_CODE_3,
    VEHICLE_TYPE_CODE_4,
    VEHICLE_TYPE_CODE_5,
    UNDEFINED // Sentinel for error handling
};

inline bool is_indexed_field(CollisionField field) {
    switch (field) {
        case CollisionField::CRASH_DATE:
        case CollisionField::ZIP_CODE:
        case CollisionField::LATITUDE:
        case CollisionField::LONGITUDE:
        case CollisionField::NUMBER_OF_PERSONS_INJURED:
        case CollisionField::NUMBER_OF_PERSONS_KILLED:
        case CollisionField::NUMBER_OF_PEDESTRIANS_INJURED:
        case CollisionField::NUMBER_OF_PEDESTRIANS_KILLED:
        case CollisionField::NUMBER_OF_CYCLIST_INJURED:
        case CollisionField::NUMBER_OF_CYCLIST_KILLED:
        case CollisionField::NUMBER_OF_MOTORIST_INJURED:
        case CollisionField::NUMBER_OF_MOTORIST_KILLED:
        case CollisionField::COLLISION_ID:
            return true;
        default:
            return false;
    }
}

enum class FieldValueType { UINT8_T, UINT32_T, SIZE_T, FLOAT, DATE, TIME, STRING };

inline FieldValueType field_to_value_type(const CollisionField collision_field) {
    switch(collision_field) {
        case CollisionField::CRASH_DATE:
            return FieldValueType::DATE;
        case CollisionField::CRASH_TIME:
            return FieldValueType::TIME;
        case CollisionField::BOROUGH:
        case CollisionField::LOCATION:
        case CollisionField::ON_STREET_NAME:
        case CollisionField::CROSS_STREET_NAME:
        case CollisionField::OFF_STREET_NAME:
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_1:
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_2:
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_3:
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_4:
        case CollisionField::CONTRIBUTING_FACTOR_VEHICLE_5:
        case CollisionField::VEHICLE_TYPE_CODE_1:
        case CollisionField::VEHICLE_TYPE_CODE_2:
        case CollisionField::VEHICLE_TYPE_CODE_3:
        case CollisionField::VEHICLE_TYPE_CODE_4:
        case CollisionField::VEHICLE_TYPE_CODE_5:
            return FieldValueType::STRING;
        case CollisionField::ZIP_CODE:
            return FieldValueType::UINT32_T;
        case CollisionField::LATITUDE:
        case CollisionField::LONGITUDE:
            return FieldValueType::FLOAT;
        case CollisionField::NUMBER_OF_PERSONS_INJURED:
        case CollisionField::NUMBER_OF_PERSONS_KILLED:
        case CollisionField::NUMBER_OF_PEDESTRIANS_INJURED:
        case CollisionField::NUMBER_OF_PEDESTRIANS_KILLED:
        case CollisionField::NUMBER_OF_CYCLIST_INJURED:
        case CollisionField::NUMBER_OF_CYCLIST_KILLED:
        case CollisionField::NUMBER_OF_MOTORIST_INJURED:
        case CollisionField::NUMBER_OF_MOTORIST_KILLED:
            return FieldValueType::UINT8_T;
        case CollisionField::COLLISION_ID:
            return FieldValueType::SIZE_T;
        case CollisionField::UNDEFINED:
        default:
            throw std::runtime_error("Unknown CollisionField was provided!");
    }
}
