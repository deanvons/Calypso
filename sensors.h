/* sensors.h -- sensor read, fault-check, and history buffer declarations */

#include <stdint.h>
#include <stdbool.h>

typedef float sensor_float_t;

/* NOTE: #define constant -- preprocessor macros are covered in Phase 15 */
#define SENSOR_HISTORY_LEN 10

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

/* --- History buffers ------------------------------------------------- */

void           sensors_record_fuel(uint16_t reading);
void           sensors_record_velocity(uint16_t reading);
sensor_float_t sensors_compute_fuel_avg(void);
// SOLUTION (Challenge 5): return index of first drifting reading, or -1 if none
int            sensors_detect_fuel_drift(void);

// SOLUTION (Challenge 4): pressure history buffer API
void           sensors_record_pressure(uint16_t reading);
sensor_float_t sensors_compute_pressure_avg(void);
