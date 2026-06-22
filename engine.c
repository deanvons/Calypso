/* engine.c -- engine control register operations */

#include <string.h>
#include "engine.h"

/*
 * ENGINE_CTRL  -- 32-bit engine control register
 *   Bits 0-3 : Thruster enable (one bit per thruster, thruster 0 = bit 0)
 *   Bits 4-7 : Throttle level (0-15)
 *   Bits 8-31: Reserved
 *
 * ENGINE_STATUS -- 32-bit engine status register
 *   Bit 0 : Sensor fault
 *   Bit 1 : Temperature warning
 *   Bit 2 : Critical fault
 *   Bits 3-31: Reserved
 *
 * Both are static: visible only within this translation unit.
 * main.c cannot read or write either register directly.
 *
 * volatile: on real hardware, the engine peripheral controller can update
 * these registers independently of this program. Without volatile, the
 * compiler is free to read a register once, cache the value in a CPU
 * register, and skip re-reading memory on later accesses -- silently
 * missing any change the peripheral made in between.
 */
static volatile uint32_t ENGINE_CTRL   = 0u;
static volatile uint32_t ENGINE_STATUS = 0u;

/*
 * volatile uint32_t * const: this pointer is fixed -- it always addresses
 * ENGINE_CTRL. The value at that address can change (the volatile says so);
 * the pointer itself cannot be reseated to a different address (the const says so).
 */
static volatile uint32_t * const pENGINE_CTRL = &ENGINE_CTRL;

/*
 * NOTE: on a real MCU, ENGINE_CTRL would not be a variable in RAM at all --
 * it would be a register at a fixed physical address the engine peripheral
 * is mapped to. Accessing it from C means casting that address to a pointer:
 *
 *   volatile uint32_t * const pMMIO_ENGINE_CTRL = (volatile uint32_t *)ENGINE_CTRL_BASE;
 *
 * Dereferencing pMMIO_ENGINE_CTRL would read/write address ENGINE_CTRL_BASE directly.
 * Left commented here -- dereferencing an arbitrary address on a desktop process
 * segfaults, because no memory is mapped there. pENGINE_CTRL above is used instead,
 * pointing at the simulated ENGINE_CTRL variable.
 */

/* Bit-position constants -- private to this file */
static const uint8_t THROTTLE_SHIFT     = 4;
static const uint8_t THROTTLE_MASK      = 0x0Fu;
static const uint8_t SENSOR_FAULT_BIT   = 0;
static const uint8_t TEMP_WARNING_BIT   = 1;
static const uint8_t CRITICAL_FAULT_BIT = 2;

/* Map thruster number 0-3 to its bit position in ENGINE_CTRL */
// NOTE: thruster >= THRUSTER_COUNT silently maps to bit 0 -- avoids undefined shift past register width
static uint8_t thruster_bit(uint8_t thruster) {
    // SOLUTION (Challenge 2): named constant instead of the bare literal 4u
    return (thruster < THRUSTER_COUNT) ? thruster : 0u;
}

void engine_enable_thruster(uint8_t thruster) {
    *pENGINE_CTRL |= ((uint32_t)1u << thruster_bit(thruster));
}

void engine_disable_thruster(uint8_t thruster) {
    *pENGINE_CTRL &= ~((uint32_t)1u << thruster_bit(thruster));
}

void engine_set_throttle(uint8_t level) {
    /*
     * SOLUTION (Challenge 5, stretch): clamp level into the register's
     * representable range before writing it -- engine_set_throttle(20) now
     * caps at 15 instead of corrupting the reserved bits above the field.
     */
    level = CLAMP(level, 0, 15);
    *pENGINE_CTRL &= ~((uint32_t)THROTTLE_MASK << THROTTLE_SHIFT);
    *pENGINE_CTRL |=  ((uint32_t)level          << THROTTLE_SHIFT);
}

uint8_t engine_read_throttle(void) {
    return (uint8_t)((*pENGINE_CTRL >> THROTTLE_SHIFT) & THROTTLE_MASK);
}

uint32_t engine_get_ctrl(void) {
    return *pENGINE_CTRL;
}

engine_ctrl_reg_t engine_read_ctrl_bits(void) {
    /*
     * memcpy copies the raw 4 bytes of the current register value into the
     * bitfield struct -- a plain assignment would also work here since both
     * are the same size, but memcpy makes the byte-for-byte overlay explicit.
     */
    uint32_t raw = *pENGINE_CTRL;
    engine_ctrl_reg_t bits;
    memcpy(&bits, &raw, sizeof(bits));
    return bits;
}

uint32_t engine_get_status(void) {
    return ENGINE_STATUS;
}

void engine_set_sensor_fault(void) {
    ENGINE_STATUS |= ((uint32_t)1u << SENSOR_FAULT_BIT);
}

void engine_set_temp_warning(void) {
    ENGINE_STATUS |= ((uint32_t)1u << TEMP_WARNING_BIT);
}

void engine_set_critical_fault(void) {
    ENGINE_STATUS |= ((uint32_t)1u << CRITICAL_FAULT_BIT);
}

bool engine_fault_critical(void) {
    return (ENGINE_STATUS & ((uint32_t)1u << CRITICAL_FAULT_BIT)) != 0;
}

void engine_clear_status(void) {
    ENGINE_STATUS = 0u;
}

void engine_reset(void) {
    *pENGINE_CTRL = 0u;
}

/*
 * SOLUTION (Challenge 3): engine_halted is volatile for the same reason
 * ENGINE_CTRL is -- main.c's while(1) loop reads it on every iteration, and
 * without volatile the compiler could legally check it once before the loop
 * begins and never re-read it from memory inside the loop body.
 */
static volatile bool engine_halted = false;

void engine_halt(void) {
    engine_halted = true;
}

bool engine_is_halted(void) {
    return engine_halted;
}
