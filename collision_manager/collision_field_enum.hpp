#pragma once

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
