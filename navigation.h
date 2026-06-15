/* navigation.h -- navigation calculation declarations */

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"   /* for sensor_float_t */

sensor_float_t nav_burn_rate(uint16_t consumed_kg, uint32_t elapsed_s);
sensor_float_t nav_hours_to_dest(uint32_t distance_km, sensor_float_t velocity_kms);
bool           nav_approach_safe(sensor_float_t velocity_kms, uint16_t fuel_kg);
// SOLUTION (Challenge 5): fuel efficiency in km per kg
sensor_float_t nav_fuel_efficiency(uint32_t distance_km, uint16_t fuel_kg);
