/* sensors.c -- sensor read and fault-check implementations */

#include "sensors.h"

sensor_float_t sensors_read_velocity(void) {
    return 32.7f;
}

uint16_t sensors_read_fuel(uint16_t tank_capacity_kg,
                            uint8_t  burn_rate_kgs,
                            uint32_t elapsed_s) {
    uint16_t consumed = (uint16_t)(burn_rate_kgs * elapsed_s);
    return tank_capacity_kg - consumed;
}

bool sensors_in_fault(sensor_float_t reading,
                       sensor_float_t low,
                       sensor_float_t high) {
    return reading < low || reading > high;
}

// SOLUTION (Challenge 4): cabin pressure sensor returning a fixed nominal value
sensor_float_t sensors_read_pressure(void) {
    return 101.325f;
}

// NOTE: 'reading' is a local copy -- adding offset here does not affect the caller's variable
uint16_t sensors_apply_calibration(uint16_t reading, uint16_t offset) {
    reading += offset;
    return reading;
}
