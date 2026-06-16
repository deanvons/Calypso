/* sensors.c -- sensor read, fault-check, and history buffer implementations */

#include <stdio.h>
#include <inttypes.h>
#include "sensors.h"

/* --- History buffers (file-scope, invisible outside this translation unit) --- */

static uint16_t fuel_history[SENSOR_HISTORY_LEN];
static uint16_t velocity_history[SENSOR_HISTORY_LEN];
static int      fuel_idx     = 0;
static int      velocity_idx = 0;

/*
 * compute_average -- static helper: receives a pointer to the first element.
 *
 * NOTE: sizeof(buf) here is the pointer size (8 bytes on 64-bit) -- NOT the
 * full array size. The array decayed to a pointer when passed; 'len' is
 * the only way to know how many elements exist.
 *
 * buf[i] and *(buf + i) are identical: the subscript operator is defined as
 * pointer arithmetic (advance i steps of sizeof(*buf) bytes) plus dereference.
 */
/* const uint16_t *: this function only reads through buf -- cannot write to it */
static sensor_float_t compute_average(const uint16_t *buf, int len) {
    sensor_float_t sum = 0.0f;
    for (int i = 0; i < len; i++) {
        sum += (sensor_float_t)buf[i];  /* buf[i] == *(buf + i) */
    }
    return sum / (sensor_float_t)len;
}

/*
 * SOLUTION (Challenge 5): returns the index of the first reading that deviates
 * from the mean by more than threshold, or -1 if no such reading exists.
 *
 * Pointer arithmetic: ptr starts at buf (first element) and advances by one
 * uint16_t per iteration. *ptr dereferences the current element -- identical
 * to buf[i] but using the pointer directly rather than an index offset.
 */
static int detect_drift(const uint16_t *buf, int len, uint16_t threshold) {
    sensor_float_t     avg = compute_average(buf, len);
    const uint16_t    *ptr = buf;         /* pointer to first element */
    const uint16_t    *end = buf + len;   /* one past the last element */
    int                i   = 0;
    for (; ptr != end; ptr++, i++) {
        sensor_float_t diff = (sensor_float_t)*ptr - avg;
        if (diff < 0.0f) diff = -diff;
        if (diff > (sensor_float_t)threshold) {
            return i;
        }
    }
    return -1;
}

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

/* --- In-place calibration -------------------------------------------- */

void sensors_calibrate(uint16_t *reading, uint16_t offset) {
    /* *reading follows the pointer to the caller's variable and modifies it */
    *reading += offset;
}

/* --- Sensor channel reconfiguration ---------------------------------- */

void sensors_configure_channel(uint16_t **channel, uint16_t *new_buf) {
    /* *channel is the caller's uint16_t * variable; assigning new_buf redirects it */
    *channel = new_buf;
}

uint16_t *sensors_fuel_history_ptr(void)     { return fuel_history; }
uint16_t *sensors_velocity_history_ptr(void) { return velocity_history; }

/* const uint16_t *: caller's pointer is read-only once passed here */
sensor_float_t sensors_compute_channel_avg(const uint16_t *channel) {
    return compute_average(channel, SENSOR_HISTORY_LEN);
}

/* --- History buffer public API --------------------------------------- */

void sensors_record_fuel(uint16_t reading) {
    fuel_history[fuel_idx] = reading;
    /*
     * sizeof(fuel_history) / sizeof(fuel_history[0]) gives the element count
     * from the array's byte size -- valid here because fuel_history is in scope
     * as the full array, not yet decayed to a pointer.
     */
    fuel_idx = (fuel_idx + 1) % (int)(sizeof(fuel_history) / sizeof(fuel_history[0]));
}

void sensors_record_velocity(uint16_t reading) {
    velocity_history[velocity_idx] = reading;
    velocity_idx = (velocity_idx + 1) % SENSOR_HISTORY_LEN;
}

sensor_float_t sensors_compute_fuel_avg(void) {
    return compute_average(fuel_history, SENSOR_HISTORY_LEN);
}

// SOLUTION (Challenge 5): return type changed from bool to int
int sensors_detect_fuel_drift(void) {
    return detect_drift(fuel_history, SENSOR_HISTORY_LEN, 50);
}

/* SOLUTION (Challenge 4): pressure history buffer */
static uint16_t pressure_history[SENSOR_HISTORY_LEN];
static int      pressure_idx = 0;

void sensors_record_pressure(uint16_t reading) {
    pressure_history[pressure_idx] = reading;
    pressure_idx = (pressure_idx + 1) % SENSOR_HISTORY_LEN;
}

sensor_float_t sensors_compute_pressure_avg(void) {
    return compute_average(pressure_history, SENSOR_HISTORY_LEN);
}
