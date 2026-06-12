#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h> // NOTE: provides bool, true, false -- C99 and later

typedef float sensor_float_t; /* alias for float used for all sensor readings */

enum MissionPhase {
    PREFLIGHT,  /* 0 -- pre-launch checks */
    LAUNCH,     /* 1 -- engine ignition and ascent */
    CRUISE,     /* 2 -- interplanetary transit */
    APPROACH,   /* 3 -- deceleration toward destination */
    DOCKED      /* 4 -- mission complete */
};

/*
 * ENGINE_CTRL -- 32-bit engine control register simulation.
 *   Bit 0  : Thruster 0 enable
 *   Bit 1  : Thruster 1 enable
 *   Bit 2  : Thruster 2 enable
 *   Bit 3  : Thruster 3 enable
 *   Bits 4-7 : Throttle level (0-15)
 *   Bits 8-31: Reserved
 *
 * ENGINE_STATUS -- 32-bit engine status register simulation.
 *   Bit 0  : Sensor fault
 *   Bit 1  : Temperature warning
 *   Bit 2  : Critical fault
 *   Bits 3-31: Reserved
 */
uint32_t ENGINE_CTRL   = 0u;
uint32_t ENGINE_STATUS = 0u;

/* Thruster bit positions in ENGINE_CTRL */
const uint8_t THRUSTER_0_BIT = 0;
const uint8_t THRUSTER_1_BIT = 1;
const uint8_t THRUSTER_2_BIT = 2;
const uint8_t THRUSTER_3_BIT = 3;

/* Throttle field in ENGINE_CTRL -- bits 4-7 */
const uint8_t THROTTLE_SHIFT = 4;
const uint8_t THROTTLE_MASK  = 0x0Fu; /* mask applied after right-shifting by THROTTLE_SHIFT */

/* Fault bit positions in ENGINE_STATUS */
const uint8_t SENSOR_FAULT_BIT   = 0;
const uint8_t TEMP_WARNING_BIT   = 1;
const uint8_t CRITICAL_FAULT_BIT = 2;

int main(void) {
    printf("=========================================\n");
    printf("  CALYPSO FLIGHT COMPUTER\n");
    printf("  Shuttle designation : CALYPSO-7\n");
    printf("  Build date          : %s\n", __DATE__); // NOTE: predefined preprocessor macro -- covered in Phase 15
    int mission_id = 7;
    printf("  Mission ID          : %d\n", mission_id);
    printf("=========================================\n\n");

    printf("Calypso online. Initialising sensor suite.\n\n");

    /* --- Fuel sensor -------------------------------------------- */

    /*
     * Declaration and initialisation on the same line. TANK_CAPACITY_KG is
     * const -- the compiler rejects any assignment to it after this point.
     * BURN_RATE_KGS is const for the same reason: the burn rate is a fixed
     * mission parameter, not a variable.
     */
    const uint16_t TANK_CAPACITY_KG = 1000; /* kg -- full tank at launch */
    const uint8_t  BURN_RATE_KGS    = 5;    /* kg per second at cruise thrust */

    /* Declaration and initialisation as two separate steps. */
    uint32_t mission_elapsed_s;
    mission_elapsed_s = 10; /* seconds of engine burn */

    uint16_t consumed_kg = (uint16_t)(BURN_RATE_KGS * mission_elapsed_s);
    uint16_t fuel_level  = TANK_CAPACITY_KG - consumed_kg; /* fixed: Phase 2 had + instead of - */

    /*
     * Range check against UINT16_MAX. raw_fuel_adc simulates a sensor ADC
     * returning a value before it has been narrowed to uint16_t -- checking
     * that it fits before the assignment prevents a silent truncation.
     * UINT16_MAX is self-documenting: it ties the limit to the type name,
     * so the guard stays correct if the type of fuel_level ever changes.
     */
    uint32_t raw_fuel_adc = 65540; // NOTE: deliberately out-of-range to exercise the guard
    if (raw_fuel_adc > UINT16_MAX) {
        printf("FAULT: fuel ADC reading (%" PRIu32 ") exceeds uint16_t range [0, %" PRIu32 "]\n\n",
               raw_fuel_adc, (uint32_t)UINT16_MAX);
    }

    printf("--- Sensor Status ---\n");
    printf("Fuel level           : %" PRIu16 " kg\n", fuel_level);

    /* --- Engine temperature sensor ------------------------------ */

    /*
     * int8_t for the temperature delta -- signed because the delta can be
     * negative when the engine is cooling. Range: INT8_MIN (-128) to
     * INT8_MAX (127) degrees K.
     */
    int8_t engine_temp_delta = -12; /* K below nominal: engine cooling */

    /*
     * Range check against INT8_MIN and INT8_MAX. raw_temp simulates a raw
     * reading from a sensor bus before narrowing to int8_t.
     */
    int raw_temp = -130; // NOTE: deliberately out-of-range to exercise the guard
    if (raw_temp < INT8_MIN || raw_temp > INT8_MAX) {
        printf("FAULT: temperature delta (%d) outside int8_t range [%d, %d]\n",
               raw_temp, INT8_MIN, INT8_MAX);
    }

    printf("Engine temp delta    : %" PRId8 " K\n", engine_temp_delta);
    printf("Mission elapsed      : %" PRIu32 " s\n\n", mission_elapsed_s);

    /* --- Distance sensor ---------------------------------------- */

    /*
     * SOLUTION (Challenge 3): uint32_t distance sensor with PRIu32 format
     * specifier and a range guard against half the type's maximum.
     * 384 400 km is roughly the Earth–Moon distance -- well below
     * UINT32_MAX / 2 (~2.1 billion), so the guard does not trigger here.
     */
    uint32_t distance_to_destination_km = 384400; /* km */
    if (distance_to_destination_km > UINT32_MAX / 2) {
        printf("FAULT: distance reading (%" PRIu32 ") exceeds UINT32_MAX / 2 (%" PRIu32 ")\n",
               distance_to_destination_km, UINT32_MAX / 2);
    }
    printf("Distance to dest     : %" PRIu32 " km\n\n", distance_to_destination_km);

    /* --- Velocity sensor (float) --------------------------------------- */

    /*
     * sensor_float_t is typedef'd to float. velocity_kms and
     * velocity_kms_precise hold the same logical value at different
     * precision: float gives ~7 significant decimal digits, double ~15.
     * The %8.2f specifier: field width 8, 2 decimal places, right-aligned.
     * The %12.8f specifier: field width 12, 8 decimal places.
     */
    sensor_float_t velocity_kms         = 32.7f;
    double         velocity_kms_precise = 32.714159265; /* double for reference precision */

    printf("Velocity (sensor)    : %8.2f km/s\n",  velocity_kms);
    printf("Velocity (precise)   : %12.8f km/s\n", velocity_kms_precise);

    /* --- Distance sensor (float AU) ------------------------------------ */

    sensor_float_t distance_au = 0.0027f; /* AU -- approx Earth-Moon distance */
    printf("Distance             : %8.4f AU\n\n", distance_au);

    /* --- Shuttle identification (char array) ---------------------------- */

    /*
     * "CAL-007" is 7 printable characters + null terminator '\0' = 8 bytes.
     * The COMMS printf demonstrates three escape sequences:
     *   \t  -- horizontal tab (aligns the log entry)
     *   \'  -- single-quote character inside a string literal
     *   \\  -- a literal backslash (one \ in source becomes two \\ to escape it)
     */
    char shuttle_id[8] = "CAL-007";
    printf("Shuttle ID           : %s\n", shuttle_id);
    printf("COMMS:\t\'%s\' status nominal -- log: calypso\\flight.log\n\n", shuttle_id);

    /* SOLUTION (Challenge 5): shuttle_id = "NEW-001" does not compile -- an array name is
       not an assignable lvalue; it is the fixed address of the first element. An individual
       element is a modifiable lvalue and can be assigned directly. */
    shuttle_id[6] = '9'; /* change '7' to '9' */
    printf("Shuttle ID (modified): %s\n\n", shuttle_id);

    /* --- Sensor fault flag (bool) and cabin pressure sensor ------------- */

    bool sensor_fault = false;

    // SOLUTION (Challenge 3): sensor_float_t printed with %10.3f; range check sets sensor_fault
    sensor_float_t cabin_pressure_kpa = 101.325f;
    printf("Cabin pressure       : %10.3f kPa\n", cabin_pressure_kpa);
    if (cabin_pressure_kpa < 80.0f || cabin_pressure_kpa > 120.0f) {
        sensor_fault = true;
        printf("FAULT: cabin pressure (%.3f kPa) outside safe range [80.000, 120.000]\n",
               cabin_pressure_kpa);
    }
    printf("Sensor fault         : %s\n\n", sensor_fault ? "true" : "false");

    /* --- Mission phase (enum) ------------------------------------------ */

    /*
     * PREFLIGHT is the integer 0. Printing (int)current_phase alongside the
     * name makes the underlying representation visible -- the enum is stored
     * as a plain int; the name exists only in the source.
     */
    enum MissionPhase current_phase = PREFLIGHT;
    printf("Mission phase        : PREFLIGHT (%d)\n\n", (int)current_phase);

    /* --- Navigation calculations --------------------------------------- */

    printf("--- Navigation ---\n");

    /* Burn rate -- explicit cast forces float division.
     * Without the cast, consumed_kg / mission_elapsed_s is integer / integer
     * and truncates toward zero. The cast promotes the numerator to
     * sensor_float_t before the division, making both operands float. */
    sensor_float_t burn_rate_kgs = (sensor_float_t)consumed_kg / mission_elapsed_s;
    printf("Burn rate            : %6.2f kg/s\n", burn_rate_kgs);

    /* Time to destination -- parentheses control evaluation order.
     * / and * share equal precedence and associate left-to-right, so without
     * parentheses distance / velocity * 3600 evaluates as
     * (distance / velocity) * 3600 -- roughly 42 million (seconds x 3600),
     * not ~3 hours.
     * Parenthesising (velocity_kms * 3600.0f) forces the multiplication first,
     * producing the correct distance / speed-in-kph form. */
    sensor_float_t hours_to_dest = (sensor_float_t)distance_to_destination_km
                                   / (velocity_kms * 3600.0f);
    printf("Hours to destination : %8.2f h\n\n", hours_to_dest);

    /* Approach safety -- relational and logical operators.
     * Each parenthesised sub-expression produces 1 or 0 (int).
     * && requires both to be true; short-circuit: if the first is false
     * the second is not evaluated. */
    bool approach_safe = (velocity_kms <= 2.0f) && (fuel_level >= 50);
    printf("Approach safe        : %s\n", approach_safe ? "YES" : "NO");
    printf("  velocity (%.2f km/s) <= 2.0   : %s\n",
           velocity_kms, (velocity_kms <= 2.0f) ? "true" : "false");
    printf("  fuel (%" PRIu16 " kg) >= 50             : %s\n\n",
           fuel_level, (fuel_level >= 50) ? "true" : "false");

    /* SOLUTION (Challenge 3): fuel_efficiency without and with explicit cast.
     * distance_to_destination_km (uint32_t) / fuel_level (uint16_t) -- both
     * integer types, so without a cast the division truncates toward zero.
     * 384400 / 950 = 404 (integer), not 404.63 (float).
     * The cast promotes the numerator to sensor_float_t before the division,
     * making both operands float and preserving the fractional part. */
    /* NOTE: integer division happens first; the outer cast converts the truncated result to float */
    sensor_float_t fuel_efficiency_int = (sensor_float_t)(distance_to_destination_km / fuel_level);
    printf("Fuel efficiency (int div) : %6.2f km/kg  (%" PRIu32 " / %" PRIu16 " = %" PRIu32 " -- truncated)\n",
           fuel_efficiency_int,
           distance_to_destination_km, fuel_level,
           distance_to_destination_km / fuel_level);

    sensor_float_t fuel_efficiency = (sensor_float_t)distance_to_destination_km / fuel_level;
    printf("Fuel efficiency (cast)    : %6.2f km/kg  (float div)\n\n", fuel_efficiency);

    /* sizeof -- compile-time operator: no code runs at runtime.
     * Cast to unsigned so %u matches on both 32- and 64-bit size_t platforms;
     * the values are small enough that no truncation occurs. */
    printf("Type sizes:\n");
    printf("  sizeof(sensor_float_t) = %u bytes\n", (unsigned)sizeof(sensor_float_t));
    printf("  sizeof(uint16_t)       = %u bytes\n", (unsigned)sizeof(uint16_t));
    printf("  sizeof(double)         = %u bytes\n\n", (unsigned)sizeof(double));

    /* --- Engine control register operations ------------------------------ */

    printf("--- Engine Control ---\n");
    printf("ENGINE_CTRL initial    : 0x%08" PRIX32 "\n", ENGINE_CTRL);

    /* Set: OR with a single-bit mask.
     * Bit N ORed with 1 becomes 1; every other bit is ORed with 0 and stays unchanged. */
    ENGINE_CTRL |= ((uint32_t)1u << THRUSTER_0_BIT);
    printf("Set thruster 0         : 0x%08" PRIX32 "  (|= 1u << %u)\n", ENGINE_CTRL, THRUSTER_0_BIT);

    ENGINE_CTRL |= ((uint32_t)1u << THRUSTER_2_BIT);
    printf("Set thruster 2         : 0x%08" PRIX32 "  (|= 1u << %u)\n", ENGINE_CTRL, THRUSTER_2_BIT);

    /* NOTE: cast to uint32_t ensures the shift operates on the full register width */
    ENGINE_CTRL |= ((uint32_t)7u << THROTTLE_SHIFT);
    printf("Set throttle = 7       : 0x%08" PRIX32 "  (|= 7u << %u)\n", ENGINE_CTRL, THROTTLE_SHIFT);

    /* Clear: AND with the bitwise complement of the mask.
     * ~(1u << N) has 0 only at bit N and 1 everywhere else.
     * A bit ANDed with 0 becomes 0; ANDed with 1, it stays unchanged.
     *
     * LEARNING MOMENT -- type width and bitwise complement:
     * 1u has type unsigned int, not uint32_t. On a C99-conformant target
     * where unsigned int is 16 bits, ~(1u << 0) produces a 16-bit 0xFFFE.
     * Zero-extending that to 32 bits on assignment gives 0x0000FFFE, which
     * ANDs bits 16-31 to zero and silently corrupts the upper half of the
     * register. The throttle-set above already carries the (uint32_t) cast
     * for this reason. Always shift on the register's own type. */
    ENGINE_CTRL &= ~((uint32_t)1u << THRUSTER_0_BIT);
    printf("Clear thruster 0       : 0x%08" PRIX32 "  (&= ~(1u << %u))\n", ENGINE_CTRL, THRUSTER_0_BIT);

    /* Toggle: XOR with the mask. Bit N XORed with 1 flips; XORed with 0, unchanged. */
    ENGINE_CTRL ^= ((uint32_t)1u << THRUSTER_2_BIT);
    printf("Toggle thruster 2 off  : 0x%08" PRIX32 "  (^= 1u << %u)\n", ENGINE_CTRL, THRUSTER_2_BIT);

    ENGINE_CTRL ^= ((uint32_t)1u << THRUSTER_2_BIT);
    printf("Toggle thruster 2 on   : 0x%08" PRIX32 "  (^= 1u << %u)\n\n", ENGINE_CTRL, THRUSTER_2_BIT);

    /* Extract throttle field: shift right to bring bits 4-7 to positions 0-3,
     * then AND with THROTTLE_MASK (0x0F) to discard everything above bit 3. */
    uint8_t throttle_level = (uint8_t)((ENGINE_CTRL >> THROTTLE_SHIFT) & THROTTLE_MASK);
    printf("Throttle level read    : %u  ((ENGINE_CTRL >> %u) & 0x%02X)\n\n",
           throttle_level, THROTTLE_SHIFT, THROTTLE_MASK);

    /* Simulate fault conditions in ENGINE_STATUS */
    ENGINE_STATUS |= ((uint32_t)1u << SENSOR_FAULT_BIT);
    ENGINE_STATUS |= ((uint32_t)1u << CRITICAL_FAULT_BIT);
    printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n", ENGINE_STATUS);

    /* Test: AND isolates the target bit; the rest become 0. != 0 converts to bool. */
    bool fault_critical = (ENGINE_STATUS & ((uint32_t)1u << CRITICAL_FAULT_BIT)) != 0;
    bool fault_sensor   = (ENGINE_STATUS & ((uint32_t)1u << SENSOR_FAULT_BIT))   != 0;
    printf("Critical fault         : %s  (bit %u)\n",
           fault_critical ? "SET" : "clear", CRITICAL_FAULT_BIT);
    printf("Sensor fault           : %s  (bit %u)\n\n",
           fault_sensor   ? "SET" : "clear", SENSOR_FAULT_BIT);

    /* SOLUTION (Challenge 3): enable thruster 1, change throttle to 10, confirm readback */
    ENGINE_CTRL |= ((uint32_t)1u << THRUSTER_1_BIT);
    printf("Set thruster 1         : 0x%08" PRIX32 "  (|= 1u << %u)\n", ENGINE_CTRL, THRUSTER_1_BIT);

    /* Clear the existing throttle field, then write 10 into bits 4-7.
     * THROTTLE_MASK (0x0Fu) is the post-shift mask; shifting it back by
     * THROTTLE_SHIFT gives the in-place mask 0xF0u that covers bits 4-7. */
    ENGINE_CTRL &= ~((uint32_t)THROTTLE_MASK << THROTTLE_SHIFT);
    ENGINE_CTRL |=  ((uint32_t)10u            << THROTTLE_SHIFT);
    printf("Set throttle = 10      : 0x%08" PRIX32 "  (cleared field, then |= 10u << %u)\n",
           ENGINE_CTRL, THROTTLE_SHIFT);

    uint8_t throttle_readback = (uint8_t)((ENGINE_CTRL >> THROTTLE_SHIFT) & THROTTLE_MASK);
    bool    thruster1_set     = (ENGINE_CTRL & ((uint32_t)1u << THRUSTER_1_BIT)) != 0;
    printf("Throttle readback      : %u  (expected 10)\n", throttle_readback);
    printf("Thruster 1 still set   : %s\n\n", thruster1_set ? "YES" : "NO");

    /*
     * SOLUTION (Challenge 5): BATTERY_LEVEL field in ENGINE_STATUS bits 4-7.
     * Bits 0-2 hold the fault flags -- the write must not disturb them.
     * The in-place mask for bits 4-7 is (BATTERY_LEVEL_MASK << BATTERY_LEVEL_SHIFT).
     */
    const uint8_t BATTERY_LEVEL_SHIFT = 4;
    const uint8_t BATTERY_LEVEL_MASK  = 0x0Fu;

    printf("ENGINE_STATUS (before) : 0x%08" PRIX32 "\n", ENGINE_STATUS);
    ENGINE_STATUS &= ~((uint32_t)BATTERY_LEVEL_MASK << BATTERY_LEVEL_SHIFT);
    ENGINE_STATUS |=  ((uint32_t)12u                << BATTERY_LEVEL_SHIFT);
    printf("ENGINE_STATUS (after)  : 0x%08" PRIX32 "  (battery = 12 in bits 4-7)\n", ENGINE_STATUS);

    uint8_t battery_level = (uint8_t)((ENGINE_STATUS >> BATTERY_LEVEL_SHIFT) & BATTERY_LEVEL_MASK);
    printf("Battery level readback : %u  (expected 12)\n\n", battery_level);

    /* --- Command loop --------------------------------------------------- */

    /* Reset ENGINE_STATUS before entering the command loop. The phase-06 register
     * demo set fault bits deliberately to demonstrate STATUS operations; clearing
     * them here puts the computer in a known state for active operation. */
    ENGINE_STATUS = 0u;

    printf("=========================================\n");
    printf("  CALYPSO COMMAND LOOP ACTIVE\n");
    printf("  n=advance phase  s=sensor scan  e=emergency  q=quit\n");
    printf("=========================================\n\n");

    while (1) {
        /* Ternary chain: produces the phase name without an if block */
        const char *phase_name =
            (current_phase == PREFLIGHT) ? "PREFLIGHT" :
            (current_phase == LAUNCH)    ? "LAUNCH"    :
            (current_phase == CRUISE)    ? "CRUISE"    :
            (current_phase == APPROACH)  ? "APPROACH"  : "DOCKED";

        /* do-while: prompt once; re-prompt if the command is unrecognised.
         * The '\0' initialiser guards against UB if scanf returns EOF on a closed
         * pipe; it is not a sentinel the loop depends on. In normal terminal
         * operation the do-while guarantee means cmd is set by scanf before use. */
        char cmd = '\0';
        do {
            printf("[%s] Command (n/s/e/q): ", phase_name);
            scanf(" %c", &cmd);
            if (cmd != 'n' && cmd != 's' && cmd != 'e' && cmd != 'q') {
                printf("Unknown command '%c'.\n", cmd);
            }
        } while (cmd != 'n' && cmd != 's' && cmd != 'e' && cmd != 'q');

        /* 'q' -- quit: break exits the while(1) command loop */
        if (cmd == 'q') {
            printf("Shutdown command received.\n");
            break;
        }

        /* 'e' -- emergency: set fault bits, then jump to the cleanup label */
        if (cmd == 'e') {
            ENGINE_STATUS |= ((uint32_t)1u << SENSOR_FAULT_BIT);
            ENGINE_STATUS |= ((uint32_t)1u << CRITICAL_FAULT_BIT);
            printf("EMERGENCY COMMAND RECEIVED -- initiating shutdown\n");
            goto emergency_shutdown;
        }

        /* 'n' -- advance mission phase */
        if (cmd == 'n') {
            /*
             * switch displays phase-specific status. Intentional fallthrough from
             * LAUNCH into CRUISE means both phases print the engine-active report.
             * Phase advancement happens after the switch, not inside it -- the
             * fallthrough body reads the register but does not modify current_phase,
             * so there is no risk of a double-advance.
             */
            switch (current_phase) {
                case PREFLIGHT:
                    printf("  Pre-flight: all systems nominal\n");
                    break;

                case LAUNCH:
                    printf("  Launch: ignition sequence active\n");
                    /* FALLTHROUGH -- LAUNCH and CRUISE both report the active-burn state */
                case CRUISE:
                    {
                        uint8_t thr = (uint8_t)((ENGINE_CTRL >> THROTTLE_SHIFT) & THROTTLE_MASK);
                        printf("  Active burn: throttle=%u | fuel=%" PRIu16 " kg | STATUS=0x%08" PRIX32 "\n",
                               thr, fuel_level, ENGINE_STATUS);
                    }
                    break;

                case APPROACH:
                    printf("  Approach: decelerating -- velocity %.2f km/s\n", velocity_kms);
                    break;

                case DOCKED:
                    printf("  Docked: mission complete -- no further advance\n");
                    break;

                default:
                    break;
            }

            /* Advance the phase; ternary names the new state for the log line */
            if (current_phase != DOCKED) {
                current_phase = (enum MissionPhase)((int)current_phase + 1);
                const char *new_name =
                    (current_phase == LAUNCH)   ? "LAUNCH"   :
                    (current_phase == CRUISE)   ? "CRUISE"   :
                    (current_phase == APPROACH) ? "APPROACH" :
                    (current_phase == DOCKED)   ? "DOCKED"   : "UNKNOWN";
                printf("  >> Phase advanced to %s\n\n", new_name);
            } else {
                printf("\n");
            }
        }

        /* 's' -- periodic sensor scan */
        if (cmd == 's') {
            /*
             * NOTE: arrays are covered formally in Phase 9 -- these small
             * fixed-size local arrays are used here purely to give the for
             * loop something to iterate over.
             */
            /* SOLUTION (Challenge 3): sensor_readings[2] is set high to demonstrate the HIGH_WARN trigger */
            sensor_float_t sensor_readings[3] = {
                cabin_pressure_kpa,
                velocity_kms,
                3200.0f
            };
            bool sensor_faults_scan[3] = {
                (cabin_pressure_kpa < 80.0f || cabin_pressure_kpa > 120.0f),
                (velocity_kms > 25.0f), /* NOTE: 32.7 km/s exceeds calibration range -- sensor faulted */
                (fuel_level < 50)
            };
            const int SENSOR_COUNT = 3;

            printf("--- Periodic Sensor Scan ---\n");
            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (sensor_faults_scan[i]) {
                    printf("  Sensor %d: FAULTED -- skipping\n", i);
                    continue; /* skip the normal reading line for this sensor */
                }
                /* SOLUTION (Challenge 3): second threshold check -- set TEMP_WARNING_BIT and continue */
                const sensor_float_t HIGH_WARN = 2000.0f;
                if (sensor_readings[i] > HIGH_WARN) {
                    ENGINE_STATUS |= ((uint32_t)1u << TEMP_WARNING_BIT);
                    printf("  Sensor %d: WARNING -- reading %.3f exceeds HIGH_WARN threshold\n",
                           i, sensor_readings[i]);
                    continue;
                }
                /* if/else if/else: classify the reading by threshold */
                const char *level;
                if (sensor_readings[i] > 500.0f) {
                    level = "HIGH";
                } else if (sensor_readings[i] > 50.0f) {
                    level = "NOMINAL";
                } else {
                    level = "LOW";
                }
                printf("  Sensor %d: %8.3f  [%s]\n", i, sensor_readings[i], level);
            }
            /* SOLUTION (Challenge 3): print ENGINE_STATUS to confirm TEMP_WARNING_BIT was set */
            printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n\n", ENGINE_STATUS);
        }

        /* End-of-cycle critical fault check -- goto if fault was set this cycle */
        bool fault_now = (ENGINE_STATUS & ((uint32_t)1u << CRITICAL_FAULT_BIT)) != 0;
        if (fault_now) {
            printf("CRITICAL FAULT DETECTED -- emergency shutdown\n");
            goto emergency_shutdown;
        }
    }

    printf("\nCalypso offline.\n");
    return 0; // NOTE: normal-quit path -- execution does not reach emergency_shutdown label below

emergency_shutdown:
    printf("\n--- EMERGENCY SHUTDOWN ---\n");
    /* SOLUTION (Challenge 5): report which thrusters were active before clearing the register */
    {
        const uint8_t thruster_bits[4] = {
            THRUSTER_0_BIT, THRUSTER_1_BIT, THRUSTER_2_BIT, THRUSTER_3_BIT
        };
        for (int i = 0; i < 4; i++) {
            if (ENGINE_CTRL & ((uint32_t)1u << thruster_bits[i])) {
                printf("  Thruster %d was active at shutdown\n", i);
            }
        }
    }
    ENGINE_CTRL = 0u;
    printf("ENGINE_CTRL cleared    : 0x%08" PRIX32 "\n", ENGINE_CTRL);
    printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n", ENGINE_STATUS);
    printf("Calypso offline.\n");
    return 0;
}
