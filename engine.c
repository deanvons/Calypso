/* engine.c -- engine control register operations */

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
 */
static uint32_t ENGINE_CTRL   = 0u;
static uint32_t ENGINE_STATUS = 0u;

/* Bit-position constants -- private to this file */
static const uint8_t THROTTLE_SHIFT     = 4;
static const uint8_t THROTTLE_MASK      = 0x0Fu;
static const uint8_t SENSOR_FAULT_BIT   = 0;
static const uint8_t TEMP_WARNING_BIT   = 1;
static const uint8_t CRITICAL_FAULT_BIT = 2;

/* Map thruster number 0-3 to its bit position in ENGINE_CTRL */
// NOTE: thruster >= 4 silently maps to bit 0 -- avoids undefined shift past register width
static uint8_t thruster_bit(uint8_t thruster) {
    return (thruster < 4u) ? thruster : 0u;
}

void engine_enable_thruster(uint8_t thruster) {
    ENGINE_CTRL |= ((uint32_t)1u << thruster_bit(thruster));
}

void engine_disable_thruster(uint8_t thruster) {
    ENGINE_CTRL &= ~((uint32_t)1u << thruster_bit(thruster));
}

void engine_set_throttle(uint8_t level) {
    ENGINE_CTRL &= ~((uint32_t)THROTTLE_MASK << THROTTLE_SHIFT);
    ENGINE_CTRL |=  ((uint32_t)level          << THROTTLE_SHIFT);
}

uint8_t engine_read_throttle(void) {
    return (uint8_t)((ENGINE_CTRL >> THROTTLE_SHIFT) & THROTTLE_MASK);
}

uint32_t engine_get_ctrl(void) {
    return ENGINE_CTRL;
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
    ENGINE_CTRL = 0u;
}
