#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "sensors.h"
#include "engine.h"
#include "navigation.h"
#include "crew.h"

enum MissionPhase {
    PREFLIGHT,  /* 0 -- pre-launch checks */
    LAUNCH,     /* 1 -- engine ignition and ascent */
    CRUISE,     /* 2 -- interplanetary transit */
    APPROACH,   /* 3 -- deceleration toward destination */
    DOCKED      /* 4 -- mission complete */
};

int main(void) {
    printf("=========================================\n");
    printf("  CALYPSO FLIGHT COMPUTER\n");
    printf("  Shuttle designation : CALYPSO-7\n");
    printf("  Build date          : %s\n", __DATE__); // NOTE: predefined preprocessor macro -- covered in Phase 15
    printf("  Mission ID          : %d\n", 7);
    printf("=========================================\n\n");

    printf("Calypso online. Initialising sensor suite.\n\n");

    /* --- String literal vs mutable char array ----------------------------- */
    /*
     * "CALYPSO-7" is a string literal: the compiler places these 10 bytes
     * (9 chars + '\0') in a read-only data segment. mission_label holds a
     * pointer to that read-only region. Writing through mission_label --
     * e.g. mission_label[0] = 'X' -- is undefined behaviour (segfault on
     * most systems because the memory page is mapped read-only).
     */
    const char *mission_label = "CALYPSO-7";

    /*
     * char[] initialises from the literal but copies the bytes onto the stack.
     * mission_id is an ordinary writable array -- mission_id[8] = '8' is safe.
     * strlen("CALYPSO-7") == 9; sizeof(mission_id) == 10 (includes '\0').
     */
    char mission_id[] = "CALYPSO-7";
    mission_id[8] = '8'; /* safely modifies the stack copy -- now "CALYPSO-8" */

    printf("Mission label (literal) : %s  (read-only; cannot be modified)\n", mission_label);
    printf("Mission ID (mutable)    : %s  (stack copy; safely modified)\n\n", mission_id);

    /* --- Mission parameters ------------------------------------------- */

    const uint16_t TANK_CAPACITY_KG = 1000;
    const uint8_t  BURN_RATE_KGS    = 5;
    const uint32_t mission_elapsed_s = 10;
    const uint32_t distance_to_dest_km = 384400;

    /* --- Sensor reads -------------------------------------------------- */

    printf("--- Sensor Status ---\n");

    uint16_t fuel     = sensors_read_fuel(TANK_CAPACITY_KG, BURN_RATE_KGS, mission_elapsed_s);
    sensor_float_t velocity = sensors_read_velocity();

    bool velocity_fault = sensors_in_fault(velocity, 0.0f, 25.0f);

    // SOLUTION (Challenge 4): replace inline literal with the sensor function
    sensor_float_t cabin_pressure = sensors_read_pressure();
    bool pressure_fault = sensors_in_fault(cabin_pressure, 80.0f, 120.0f);

    printf("Fuel level           : %" PRIu16 " kg\n", fuel);
    printf("Velocity             : %8.2f km/s  [%s]\n",
           velocity, velocity_fault ? "FAULT" : "OK");
    printf("Cabin pressure       : %10.3f kPa  [%s]\n\n",
           cabin_pressure, pressure_fault ? "FAULT" : "OK");

    /* --- Pass-by-value demonstration ---------------------------------- */
    /*
     * sensors_apply_calibration receives a copy of fuel -- the parameter
     * 'reading' inside the function is a separate variable. Adding the
     * offset modifies that copy; fuel in this scope is unchanged.
     */
    uint16_t fuel_calibrated = sensors_apply_calibration(fuel, 5);
    printf("Fuel (original)      : %" PRIu16 " kg  (unchanged after sensors_apply_calibration)\n", fuel);
    printf("Fuel (calibrated)    : %" PRIu16 " kg  (copy returned by the function)\n", fuel_calibrated);

    /* --- In-place calibration (pointer) -------------------------------- */
    /*
     * DELIBERATE (not executed): the three pointer pitfalls.
     *
     *   uint16_t *uninit;          // uninitialised -- holds a garbage address
     *   *uninit = 42;              // UB: writes to an unknown memory location
     *
     *   uint16_t *null_ptr = NULL; // null -- address 0, a known-invalid value
     *   *null_ptr = 42;            // crash: NULL dereference; test first: ptr != NULL
     *
     *   uint16_t *dangling;
     *   { uint16_t local = 5; dangling = &local; }
     *   printf("%u\n", *dangling); // UB: local is gone; dangling holds a dead address
     *
     * NULL is detectable before dereference (ptr != NULL).
     * Uninitialised and dangling pointers are not -- they hold addresses that look valid.
     */
    /*
     * sensors_calibrate receives &fuel_cal -- the address of a local variable.
     * *reading += offset inside the function writes through that address, modifying
     * fuel_cal directly. No return value: the side effect is the result.
     */
    uint16_t fuel_cal = fuel;
    sensors_calibrate(&fuel_cal, 5);
    printf("Fuel (in-place, ptr) : %" PRIu16 " kg  (sensors_calibrate wrote through &fuel_cal)\n\n", fuel_cal);

    // SOLUTION (Challenge 4): velocity calibration using same pass-by-pointer pattern
    uint16_t vel_cal = (uint16_t)velocity;
    sensors_calibrate_velocity(&vel_cal, 2);
    printf("Velocity (in-place)  : %" PRIu16 " km/s  (sensors_calibrate_velocity wrote through &vel_cal)\n\n", vel_cal);

    /* --- Navigation ---------------------------------------------------- */

    printf("--- Navigation ---\n");

    uint16_t consumed_kg = (uint16_t)(BURN_RATE_KGS * mission_elapsed_s);
    sensor_float_t burn_rate = nav_burn_rate(consumed_kg, mission_elapsed_s);
    printf("Burn rate            : %6.2f kg/s\n", burn_rate);

    sensor_float_t hours = nav_hours_to_dest(distance_to_dest_km, velocity);
    printf("Hours to destination : %8.2f h\n", hours);

    bool approach_safe = nav_approach_safe(velocity, fuel);
    printf("Approach safe        : %s\n", approach_safe ? "YES" : "NO");

    // SOLUTION (Challenge 5): fuel efficiency calculation
    sensor_float_t efficiency = nav_fuel_efficiency(distance_to_dest_km, fuel);
    printf("Fuel efficiency      : %8.2f km/kg\n\n", efficiency);

    /* --- Engine control ------------------------------------------------ */

    printf("--- Engine Control ---\n");
    printf("ENGINE_CTRL initial    : 0x%08" PRIX32 "\n", engine_get_ctrl());

    engine_enable_thruster(0);
    printf("Enable thruster 0      : 0x%08" PRIX32 "\n", engine_get_ctrl());

    engine_enable_thruster(2);
    printf("Enable thruster 2      : 0x%08" PRIX32 "\n", engine_get_ctrl());

    engine_set_throttle(7);
    printf("Set throttle = 7       : 0x%08" PRIX32 "\n", engine_get_ctrl());

    engine_disable_thruster(0);
    printf("Disable thruster 0     : 0x%08" PRIX32 "\n", engine_get_ctrl());

    printf("Throttle read          : %u\n", engine_read_throttle());

    engine_set_sensor_fault();
    engine_set_critical_fault();
    printf("ENGINE_STATUS (faults) : 0x%08" PRIX32 "\n", engine_get_status());
    printf("Critical fault         : %s\n", engine_fault_critical() ? "SET" : "clear");

    engine_enable_thruster(1);
    engine_set_throttle(10);
    printf("Thruster 1, throttle 10: CTRL=0x%08" PRIX32 "\n\n", engine_get_ctrl());

    engine_clear_status(); /* reset STATUS before entering the command loop */

    /* --- Crew manifest ------------------------------------------------- */

    crew_init();
    crew_set_name(0, "CHEN");    crew_set_id(0, 101);
    crew_set_name(1, "VASQUEZ"); crew_set_id(1, 102);
    crew_set_name(2, "PARK");    crew_set_id(2, 103);

    printf("--- Crew Identification ---\n");

    /*
     * strncat(dest, src, n): appends at most n bytes of src to dest, then
     * writes '\0'. The bound is sizeof(comms_buf) - strlen(comms_buf) - 1:
     * remaining capacity minus one byte reserved for the null terminator.
     * Without the bound, strncat would be as unsafe as strcat.
     */
    char comms_buf[48] = "COMMS: ";
    strncat(comms_buf, crew_get_name(0), sizeof(comms_buf) - strlen(comms_buf) - 1);
    printf("Comms transmission      : %s\n", comms_buf);

    /* crew_find_by_name uses strcmp -- compares byte sequences, not addresses */
    int found = crew_find_by_name("PARK");
    printf("Lookup 'PARK'           : slot %d\n", found);
    found = crew_find_by_name("UNKNOWN_CREW");
    printf("Lookup 'UNKNOWN_CREW'   : slot %d (not found)\n\n", found);

    /* --- Command loop -------------------------------------------------- */

    enum MissionPhase current_phase = PREFLIGHT;

    printf("=========================================\n");
    printf("  CALYPSO COMMAND LOOP ACTIVE\n");
    printf("  n=advance phase  s=sensor scan  m=manifest  e=emergency  q=quit\n");
    printf("=========================================\n\n");

    while (1) {
        /* Ternary chain: produce the phase name without an if block */
        const char *phase_name =
            (current_phase == PREFLIGHT) ? "PREFLIGHT" :
            (current_phase == LAUNCH)    ? "LAUNCH"    :
            (current_phase == CRUISE)    ? "CRUISE"    :
            (current_phase == APPROACH)  ? "APPROACH"  : "DOCKED";

        /* do-while: prompt once; re-prompt on unrecognised input */
        // NOTE: '\0' guards against UB if scanf returns EOF without writing to cmd
        char cmd = '\0';
        do {
            printf("[%s] Command (n/s/m/e/q): ", phase_name);
            scanf(" %c", &cmd);
            if (cmd != 'n' && cmd != 's' && cmd != 'm' && cmd != 'e' && cmd != 'q') {
                printf("Unknown command '%c'.\n", cmd);
            }
        } while (cmd != 'n' && cmd != 's' && cmd != 'm' && cmd != 'e' && cmd != 'q');

        if (cmd == 'q') {
            printf("Shutdown command received.\n");
            break;
        }

        if (cmd == 'e') {
            engine_set_sensor_fault();
            engine_set_critical_fault();
            printf("EMERGENCY COMMAND RECEIVED -- initiating shutdown\n");
            goto emergency_shutdown;
        }

        if (cmd == 'n') {
            /*
             * switch on enum constant: intentional fallthrough from LAUNCH into
             * CRUISE -- both phases report the same active-burn state.
             */
            switch (current_phase) {
                case PREFLIGHT:
                    printf("  Pre-flight: all systems nominal\n");
                    break;

                case LAUNCH:
                    printf("  Launch: ignition sequence active\n");
                    /* FALLTHROUGH -- LAUNCH and CRUISE share the active-burn report */
                case CRUISE:
                    {
                        uint8_t thr = engine_read_throttle();
                        printf("  Active burn: throttle=%u | fuel=%" PRIu16 " kg | STATUS=0x%08" PRIX32 "\n",
                               thr, fuel, engine_get_status());
                    }
                    break;

                case APPROACH:
                    printf("  Approach: decelerating -- velocity %.2f km/s\n", velocity);
                    break;

                case DOCKED:
                    printf("  Docked: mission complete -- no further advance\n");
                    break;

                default:
                    break;
            }

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

        if (cmd == 'm') {
            crew_print_manifest();

            /* second strncat demo: build a multi-part comms line */
            char manifest_line[64] = "TX[";
            strncat(manifest_line, crew_get_name(0), sizeof(manifest_line) - strlen(manifest_line) - 1);
            strncat(manifest_line, "]",               sizeof(manifest_line) - strlen(manifest_line) - 1);
            printf("Comms line              : %s  (len=%zu)\n\n",
                   manifest_line, strlen(manifest_line));
        }

        if (cmd == 's') {
            /* NOTE: readings[2] = 3200.0f is a demonstration value to show the HIGH_WARN path */
            sensor_float_t readings[3] = { cabin_pressure, velocity, 3200.0f };
            bool faults[3] = {
                sensors_in_fault(readings[0], 80.0f,  120.0f),
                sensors_in_fault(readings[1], 0.0f,    25.0f),
                sensors_in_fault(readings[2], 50.0f, 10000.0f)
            };
            const int SENSOR_COUNT = 3;

            printf("--- Periodic Sensor Scan ---\n");
            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (faults[i]) {
                    printf("  Sensor %d: FAULTED -- skipping\n", i);
                    continue;
                }
                const sensor_float_t HIGH_WARN = 2000.0f;
                if (readings[i] > HIGH_WARN) {
                    engine_set_temp_warning();
                    printf("  Sensor %d: WARNING -- reading %.3f exceeds HIGH_WARN threshold\n",
                           i, readings[i]);
                    continue;
                }
                const char *level;
                if      (readings[i] > 500.0f) level = "HIGH";
                else if (readings[i] > 50.0f)  level = "NOMINAL";
                else                            level = "LOW";
                printf("  Sensor %d: %8.3f  [%s]\n", i, readings[i], level);
            }
            printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n", engine_get_status());

            /* record current readings into the circular history buffers */
            sensors_record_fuel(fuel);
            // NOTE: float-to-integer cast truncates toward zero (32.7f → 32); precision loss is intentional
            sensors_record_velocity((uint16_t)velocity);

            sensor_float_t fuel_avg   = sensors_compute_fuel_avg();
            // SOLUTION (Challenge 5): drift returns the drifting index, or -1
            int            drift_idx  = sensors_detect_fuel_drift();
            printf("Fuel avg (history)     : %.1f kg  |  drift at index: %d\n",
                   fuel_avg, drift_idx);
            // SOLUTION (Challenge 5): pointer-based history walk with addresses
            sensors_print_history_ptr(sensors_fuel_history_ptr(), SENSOR_HISTORY_LEN);

            // SOLUTION (Challenge 4): record pressure into history and print average
            // NOTE: float-to-integer cast truncates toward zero (101.325f → 101)
            sensors_record_pressure((uint16_t)cabin_pressure);
            sensor_float_t pressure_avg = sensors_compute_pressure_avg();
            printf("Pressure avg (history) : %.1f kPa\n", pressure_avg);

            /*
             * Sensor channel reconfiguration: sensors_configure_channel takes uint16_t **
             * so it can redirect what primary_channel points to. Passing &primary_channel
             * gives the function the address of the pointer -- *channel = new_buf inside
             * the function changes the pointer itself, not the value it pointed to.
             */
            uint16_t *primary_channel = NULL;
            sensors_configure_channel(&primary_channel, sensors_fuel_history_ptr());
            printf("Channel -> fuel        : %.1f kg avg\n", sensors_compute_channel_avg(primary_channel));
            sensors_configure_channel(&primary_channel, sensors_velocity_history_ptr());
            printf("Channel -> velocity    : %.1f avg\n\n", sensors_compute_channel_avg(primary_channel));
        }

        if (engine_fault_critical()) {
            printf("CRITICAL FAULT DETECTED -- emergency shutdown\n");
            goto emergency_shutdown;
        }
    }

    printf("\nCalypso offline.\n");
    return 0; // NOTE: normal-quit path -- execution does not reach emergency_shutdown label below

emergency_shutdown:
    engine_reset();
    printf("\n--- EMERGENCY SHUTDOWN ---\n");
    printf("ENGINE_CTRL cleared    : 0x%08" PRIX32 "\n", engine_get_ctrl());
    printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n", engine_get_status());
    printf("Calypso offline.\n");
    return 0;
}
