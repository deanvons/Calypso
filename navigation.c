/* navigation.c -- navigation calculation implementations */

#include "navigation.h"

sensor_float_t nav_burn_rate(uint16_t consumed_kg, uint32_t elapsed_s) {
    return (sensor_float_t)consumed_kg / (sensor_float_t)elapsed_s;
}

/* NOTE: parentheses force the multiplication before the division -- see Phase 5 explanation */
sensor_float_t nav_hours_to_dest(uint32_t distance_km, sensor_float_t velocity_kms) {
    return (sensor_float_t)distance_km / (velocity_kms * 3600.0f);
}

bool nav_approach_safe(sensor_float_t velocity_kms, uint16_t fuel_kg) {
    return (velocity_kms <= 2.0f) && (fuel_kg >= 50);
}
