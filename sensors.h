/* sensors.h -- sensor read, fault-check, and history buffer declarations */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef float sensor_float_t;

#define SENSOR_HISTORY_LEN 10

/*
 * Sensor fault-range thresholds -- named once here instead of typed out at
 * every fault check. Both the boot-time check and the periodic scan in
 * main.c read these same four constants, so widening a tolerance means
 * changing one line instead of finding and updating every call site by hand.
 */
#define SENSOR_VELOCITY_FAULT_LOW    0.0f
#define SENSOR_VELOCITY_FAULT_HIGH  25.0f
#define SENSOR_PRESSURE_FAULT_LOW   80.0f
#define SENSOR_PRESSURE_FAULT_HIGH 120.0f

/*
 * ASSERT_SENSOR_RANGE(val, min, max): warns on stderr, naming the file and
 * line of the call site, if val falls outside [min, max]. This has to be a
 * macro, not a function -- __FILE__ and __LINE__ expand wherever the macro
 * is pasted, so each call site reports its own location. A function's
 * __FILE__/__LINE__ would always report the function's own definition site,
 * no matter which of main.c's call sites triggered it.
 *
 * NOTE: val appears twice in the expansion below (once per bound check).
 * Safe with a plain variable; passing an expression with a side effect
 * (a function call) would silently evaluate it twice.
 */
#define ASSERT_SENSOR_RANGE(val, min, max)                                  \
    do {                                                                    \
        if ((val) < (min) || (val) > (max)) {                               \
            fprintf(stderr,                                                 \
                    "ASSERT_SENSOR_RANGE failed at %s:%d -- value %.3f out of [%.3f, %.3f]\n", \
                    __FILE__, __LINE__, (double)(val), (double)(min), (double)(max)); \
        }                                                                    \
    } while (0)

/* --- Sensor reads ---------------------------------------------------- */

sensor_float_t sensors_read_velocity(void);
uint16_t       sensors_read_fuel(uint16_t tank_capacity_kg,
                                  uint8_t  burn_rate_kgs,
                                  uint32_t elapsed_s);
bool           sensors_in_fault(sensor_float_t reading,
                                 sensor_float_t low,
                                 sensor_float_t high);
// SOLUTION (Challenge 4): prototype for cabin pressure sensor function
sensor_float_t sensors_read_pressure(void);

/*
 * sensors_apply_calibration -- pass-by-value demonstration.
 * C copies 'reading' into a local parameter. The function modifies its
 * own copy; the caller's variable is unchanged after the call returns.
 */
uint16_t       sensors_apply_calibration(uint16_t reading, uint16_t offset);

/* --- In-place calibration -------------------------------------------- */

/*
 * sensors_calibrate -- modifies *reading in place.
 * Takes the address of the caller's variable; *reading += offset writes
 * through the pointer. No return value: the side effect is the result.
 */
void           sensors_calibrate(uint16_t *reading, uint16_t offset);
// SOLUTION (Challenge 4): velocity calibration -- same pass-by-pointer pattern
void           sensors_calibrate_velocity(uint16_t *reading, uint16_t offset);

/* --- Sensor channel reconfiguration (pointer-to-pointer) ------------- */

/*
 * sensors_configure_channel -- redirects *channel to point at new_buf.
 * Takes uint16_t ** so it can modify the caller's pointer variable.
 * Without **, the function would only receive a copy of the pointer.
 */
void           sensors_configure_channel(uint16_t **channel, uint16_t *new_buf);
uint16_t      *sensors_fuel_history_ptr(void);
uint16_t      *sensors_velocity_history_ptr(void);
sensor_float_t sensors_compute_channel_avg(const uint16_t *channel);

/* --- History buffers ------------------------------------------------- */

void           sensors_record_fuel(uint16_t reading);
void           sensors_record_velocity(uint16_t reading);
sensor_float_t sensors_compute_fuel_avg(void);
// SOLUTION (Challenge 5): return index of first drifting reading, or -1 if none
int            sensors_detect_fuel_drift(void);

// SOLUTION (Challenge 4): pressure history buffer API
void           sensors_record_pressure(uint16_t reading);
sensor_float_t sensors_compute_pressure_avg(void);

/*
 * sensors_print_history_ptr -- SOLUTION (Challenge 5, stretch).
 * Iterates buf using a pointer variable (ptr++, not index notation)
 * and prints each element's value and memory address.
 */
// SOLUTION (Challenge 5): pointer-based history walk with address output
void           sensors_print_history_ptr(const uint16_t *buf, int len);

#endif /* SENSORS_H */
