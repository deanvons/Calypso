/* engine.h -- engine register control declarations */
/*
 * ENGINE_CTRL and ENGINE_STATUS are declared static in engine.c.
 * They are not visible here and cannot be accessed directly by callers.
 * All register access goes through the functions below.
 */

#include <stdint.h>
#include <stdbool.h>

/*
 * engine_ctrl_reg_t: maps the ENGINE_CTRL bit layout onto named fields.
 * Replaces shift-and-mask arithmetic ((reg >> 4) & 0xF) with bits.throttle.
 *
 * NOTE: bitfield layout within a storage unit is implementation-defined --
 * the C standard does not guarantee field order or packing. This layout is
 * correct for little-endian targets with GCC and Clang (every platform
 * Calypso targets) but is not guaranteed portable to other compilers or
 * big-endian architectures.
 */
typedef struct {
    uint32_t thrusters : 4;  /* bits 0-3: one enable bit per thruster pair */
    uint32_t throttle  : 4;  /* bits 4-7: throttle level 0-15 */
    uint32_t reserved  : 24; /* bits 8-31: reserved */
} engine_ctrl_reg_t;

void     engine_enable_thruster(uint8_t thruster);
void     engine_disable_thruster(uint8_t thruster);
void     engine_set_throttle(uint8_t level);
uint8_t  engine_read_throttle(void);
uint32_t engine_get_ctrl(void);
uint32_t engine_get_status(void);

/* engine_read_ctrl_bits: returns the current ENGINE_CTRL value as named bitfields. */
engine_ctrl_reg_t engine_read_ctrl_bits(void);

void     engine_set_sensor_fault(void);
void     engine_set_temp_warning(void);
void     engine_set_critical_fault(void);
bool     engine_fault_critical(void);
void     engine_clear_status(void);
void     engine_reset(void);

/* SOLUTION (Challenge 3): mission-complete halt flag, set by main.c on DOCKED */
void     engine_halt(void);
bool     engine_is_halted(void);
