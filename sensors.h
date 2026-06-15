/* sensors.h -- sensor read and fault-check declarations */

#include <stdint.h>
#include <stdbool.h>

typedef float sensor_float_t;

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
