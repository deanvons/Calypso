/* engine.h -- engine register control declarations */
/*
 * ENGINE_CTRL and ENGINE_STATUS are declared static in engine.c.
 * They are not visible here and cannot be accessed directly by callers.
 * All register access goes through the functions below.
 */

#include <stdint.h>
#include <stdbool.h>

void     engine_enable_thruster(uint8_t thruster);
void     engine_disable_thruster(uint8_t thruster);
void     engine_set_throttle(uint8_t level);
uint8_t  engine_read_throttle(void);
uint32_t engine_get_ctrl(void);
uint32_t engine_get_status(void);
void     engine_set_sensor_fault(void);
void     engine_set_temp_warning(void);
void     engine_set_critical_fault(void);
bool     engine_fault_critical(void);
void     engine_clear_status(void);
void     engine_reset(void);
