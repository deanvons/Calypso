#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "sensors.h"
#include "engine.h"
#include "navigation.h"
#include "crew.h"
#include "log.h"

enum MissionPhase {
    PREFLIGHT,  /* 0 -- pre-launch checks */
    LAUNCH,     /* 1 -- engine ignition and ascent */
    CRUISE,     /* 2 -- interplanetary transit */
    APPROACH,   /* 3 -- deceleration toward destination */
    DOCKED      /* 4 -- mission complete */
};

/* --- Spacecraft struct (Phase 12 demo) ------------------------------------ */

typedef struct {
    float x_au;
    float y_au;
} position_t;

/*
 * spacecraft_t groups mission state and sensor snapshot into one object.
 * position is a nested struct -- a field whose type is itself a struct.
 * Access the inner field with a chain of dots: sc.position.x_au.
 */
typedef struct {
    char              shuttle_id[16];
    enum MissionPhase phase;
    sensor_float_t    velocity;
    uint16_t          fuel;
    position_t        position;
} spacecraft_t;

/*
 * spacecraft_print_status receives a pointer to spacecraft_t.
 * Arrow notation (sc->field) is shorthand for (*sc).field.
 * All reads go through the pointer -- no copy of the struct is made.
 */
static void spacecraft_print_status(spacecraft_t *sc) {
    const char *phase_name =
        (sc->phase == PREFLIGHT) ? "PREFLIGHT" :
        (sc->phase == LAUNCH)    ? "LAUNCH"    :
        (sc->phase == CRUISE)    ? "CRUISE"    :
        (sc->phase == APPROACH)  ? "APPROACH"  : "DOCKED";
    printf("  Shuttle       : %s\n",        sc->shuttle_id);
    printf("  Phase         : %s\n",        phase_name);
    printf("  Velocity      : %.2f km/s\n", sc->velocity);
    printf("  Fuel          : %" PRIu16 " kg\n", sc->fuel);
    /* sc->position.x_au: arrow to reach position, then dot to reach the nested field */
    printf("  Position      : (%.3f, %.3f) AU\n", sc->position.x_au, sc->position.y_au);
}

/*
 * SOLUTION (Challenge 5): append entry to a dynamically-sized string buffer.
 * Doubles the buffer with realloc whenever the entry would not fit.
 * Safe realloc pattern: assigns to tmp first; on failure, *buf remains valid.
 */
static void log_append(char **buf, size_t *cap, size_t *len, const char *entry) {
    size_t entry_len = strlen(entry);
    while (*len + entry_len + 1 > *cap) {
        size_t new_cap = *cap * 2;
        char  *tmp     = realloc(*buf, new_cap);
        if (tmp == NULL) {
            fprintf(stderr, "log_append: realloc failed\n");
            return;
        }
        *buf = tmp;
        *cap = new_cap;
    }
    strncat(*buf, entry, *cap - *len - 1);
    *len += entry_len;
}

/*
 * BOOT_CONFIG: read-only boot configuration data -- shuttle prefix, mission
 * ID, and boot flags, fixed at build time and never written at runtime.
 * const tells the compiler to reject writes and tells the linker it can
 * place the data in a read-only segment. On an embedded target, that
 * segment is flash/ROM rather than RAM -- the data survives a power cycle
 * without consuming any of the limited RAM budget.
 */
static const uint8_t BOOT_CONFIG[] = {
    0x43u, 0x41u, 0x4Cu, /* 'C','A','L' -- shuttle designation prefix */
    0x07u,               /* mission ID */
    0x01u                /* boot flags: primary sensor suite enabled */
};

int main(void) {
    /* --- Persistent flight log (Phase 16) ------------------------------ */
    /*
     * Everything printed so far lived only in this process's memory and
     * vanished the moment the program exited. log_scan_for_anomaly() and
     * log_load_checkpoint() read what the *previous* run left on disk --
     * before this run has written anything of its own.
     */
    printf("Checking persistent flight log...\n");
    log_scan_for_anomaly("calypso.log");

    checkpoint_t prev_checkpoint;
    if (log_load_checkpoint("calypso.chk", &prev_checkpoint)) {
        printf("  Previous checkpoint found: phase=%d  fuel=%" PRIu16 " kg  velocity=%.2f km/s  crew=%d\n\n",
               prev_checkpoint.mission_phase, prev_checkpoint.fuel_kg,
               prev_checkpoint.velocity_kms, prev_checkpoint.crew_count);
    } else {
        printf("  No previous checkpoint found.\n\n");
    }

    /* NOTE: log_open's bool return is checked, but a failed text log is not fatal --
       log_write()/log_write_raw() below silently no-op while log_fp is NULL. */
    if (!log_open("calypso.log")) {
        printf("  Continuing without persistent text log this session.\n\n");
    } else {
        log_write_raw("=== MISSION START ===\n");
    }

    printf("=========================================\n");
    printf("  CALYPSO FLIGHT COMPUTER\n");
    printf("  Shuttle designation : CALYPSO-7\n");
    printf("  Build date          : %s\n", __DATE__); // NOTE: predefined preprocessor macro -- covered in Phase 15
    printf("  Mission ID          : %d\n", 7);
#ifdef DEBUG_TELEMETRY
    /* SOLUTION (Challenge 3): boot-time ROM dump is debug-only, same pattern as the periodic scan's debug line */
    printf("  Boot config (ROM)   : ");
    for (size_t i = 0; i < sizeof(BOOT_CONFIG); i++) {
        printf("%02X ", BOOT_CONFIG[i]);
    }
    printf("\n");
#endif
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

    /* NOTE: thresholds are named constants from sensors.h -- the periodic scan below reads the same four */
    bool velocity_fault = sensors_in_fault(velocity, SENSOR_VELOCITY_FAULT_LOW, SENSOR_VELOCITY_FAULT_HIGH);

    // SOLUTION (Challenge 4): replace inline literal with the sensor function
    sensor_float_t cabin_pressure = sensors_read_pressure();
    bool pressure_fault = sensors_in_fault(cabin_pressure, SENSOR_PRESSURE_FAULT_LOW, SENSOR_PRESSURE_FAULT_HIGH);

    ASSERT_SENSOR_RANGE(velocity, SENSOR_VELOCITY_FAULT_LOW, SENSOR_VELOCITY_FAULT_HIGH);
    ASSERT_SENSOR_RANGE(cabin_pressure, SENSOR_PRESSURE_FAULT_LOW, SENSOR_PRESSURE_FAULT_HIGH);

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

    /*
     * SOLUTION (Challenge 5, stretch): read the same register through the
     * bitfield struct and print thrusters/throttle alongside the raw hex --
     * bits.throttle should match engine_read_throttle() exactly.
     */
    engine_ctrl_reg_t bits = engine_read_ctrl_bits();
    printf("ENGINE_CTRL bits: raw=0x%08" PRIX32 "  thrusters=%u  throttle=%u\n",
           engine_get_ctrl(), bits.thrusters, bits.throttle);

    /* SOLUTION (Challenge 5, stretch): CLAMP caps an out-of-range request instead of corrupting the reserved bits above the throttle field */
    engine_set_throttle(20);
    printf("Set throttle = 20      : 0x%08" PRIX32 "  (clamped to %u)\n",
           engine_get_ctrl(), engine_read_throttle());

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

    /* --- Crew manifest and mission log (Phase 13: dynamic allocation) -------- */

    /*
     * crew_init: allocates the roster on the heap with calloc.
     * calloc zeroes all bytes; the roster starts in a known, clean state.
     * Internal INITIAL_CAP is 2 -- adding a third member triggers the first realloc.
     */
    crew_init();
    printf("  [roster init]  capacity=%d (calloc)\n", crew_capacity());

    /*
     * Mission log buffer: malloc reserves uninitialized bytes.
     * We set log_buf[0] = '\0' immediately so the buffer starts as an empty
     * C string. malloc is used here (not calloc) to contrast with crew_init.
     */
    size_t log_cap = 32;
    size_t log_len = 0;
    char  *log_buf = malloc(log_cap);
    if (log_buf == NULL) {
        fprintf(stderr, "log_buf: allocation failed\n");
        /* NOTE: log_open() already succeeded above -- this early return is an
           existing exit path that gained a new resource to close when log.c was added. */
        log_close();
        crew_free();
        return 1;
    }
    log_buf[0] = '\0';

    log_append(&log_buf, &log_cap, &log_len, "BOOT: Calypso online\n");
    /* NOTE: log_write mirrors log_append's entry to disk -- log_buf still vanishes
       on exit (Phase 13); calypso.log does not. */
    log_write("BOOT: Calypso online");

    /* crew_add: appends one member; reallocs the roster when capacity is reached */
    crew_add("CHEN",    RANK_COMMANDER, 101, ASSIGN_FLIGHT);
    log_append(&log_buf, &log_cap, &log_len, "CREW: CHEN loaded\n");
    log_write("CREW: CHEN loaded");

    crew_add("VASQUEZ", RANK_PILOT,     102, ASSIGN_FLIGHT);
    log_append(&log_buf, &log_cap, &log_len, "CREW: VASQUEZ loaded\n");
    log_write("CREW: VASQUEZ loaded");

    /* loaded==capacity (2==2): crew_add reallocs to 4 before inserting PARK */
    crew_add("PARK",    RANK_ENGINEER,  103, ASSIGN_FLIGHT);
    log_append(&log_buf, &log_cap, &log_len, "CREW: PARK loaded\n");
    log_write("CREW: PARK loaded");

    printf("  [after 3 crew] capacity=%d (realloc: 2 -> 4)\n\n", crew_capacity());

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
    printf("Lookup 'UNKNOWN_CREW'   : slot %d (not found)\n", found);

    /*
     * crew_get_member returns a copy by value -- roster[0]'s fields are copied
     * into m. crew_print_member prints from m; any change inside that function
     * to m would not affect the roster.
     */
    printf("Print member (by value) :\n");
    crew_print_member(crew_get_member(0));

    /*
     * crew_get_member_ptr returns &roster[0] -- the address of the live slot.
     * crew_reassign writes through the pointer (m->assignment = ...), so
     * the change is visible in the roster after the function returns.
     */
    crew_reassign(crew_get_member_ptr(0), ASSIGN_SCIENCE);
    printf("After reassign (by ptr) :\n");
    crew_print_member(crew_get_member(0));
    printf("\n");

    /* SOLUTION (Challenge 4): look up slot by numeric ID using dot notation */
    int found_by_id = crew_find_by_id((uint8_t)102);
    printf("Lookup by id 102        : slot %d\n", found_by_id);
    found_by_id = crew_find_by_id((uint8_t)255); /* no crew has this id */
    printf("Lookup by id 255        : slot %d (not found)\n\n", found_by_id);

    /*
     * SOLUTION (Challenge 5): crew_update_rank takes a pointer so arrow notation
     * writes directly into the live roster slot. A by-value parameter would copy
     * the struct and the rank change would be discarded on return.
     */
    crew_update_rank(crew_get_member_ptr(2), RANK_PILOT);
    printf("After rank update (ptr) :\n");
    crew_print_member(crew_get_member(2));
    printf("\n");

    /* --- Docking transfer ------------------------------------------------- */
    /*
     * Simulates additional crew boarding during a docking manoeuvre.
     * 3 loaded / 4 capacity before transfer.
     * OKAFOR fills the last slot (no realloc).
     * PETROV: loaded==capacity (4==4) -- crew_add reallocs to 8 before inserting.
     */
    printf("--- Docking Transfer ---\n");
    printf("  Before: %d loaded / %d capacity\n", crew_count(), crew_capacity());

    crew_add("OKAFOR", RANK_SCIENTIST, 104, ASSIGN_SCIENCE);
    log_append(&log_buf, &log_cap, &log_len, "DOCK: OKAFOR transferred aboard\n");
    log_write("DOCK: OKAFOR transferred aboard");

    crew_add("PETROV",  RANK_MEDIC,     105, ASSIGN_MEDICAL);
    log_append(&log_buf, &log_cap, &log_len, "DOCK: PETROV transferred aboard\n");
    log_write("DOCK: PETROV transferred aboard");

    printf("  After:  %d loaded / %d capacity (realloc: 4 -> 8)\n\n",
           crew_count(), crew_capacity());

    /* SOLUTION (Challenge 4): shrink the roster to release unused capacity */
    printf("  Shrink: %d capacity", crew_capacity());
    crew_shrink();
    printf(" -> %d capacity (realloc: 8 -> 5)\n\n", crew_capacity());

    printf("--- Mission Log ---\n");
    printf("  buffer: %zu bytes capacity | %zu bytes used\n", log_cap, log_len);
    printf("%s\n", log_buf);

    /* --- Spacecraft record ----------------------------------------------- */
    /*
     * Dot notation: sc.shuttle_id, sc.fuel access fields directly on the value.
     * Nested struct: sc.position.x_au -- one dot per level of nesting.
     * spacecraft_print_status receives &sc -- the function uses arrow notation
     * (sc->field) to read through the pointer without copying the struct.
     */
    spacecraft_t sc;
    strncpy(sc.shuttle_id, "CALYPSO-7", sizeof(sc.shuttle_id) - 1);
    sc.shuttle_id[sizeof(sc.shuttle_id) - 1] = '\0';
    sc.phase        = PREFLIGHT;
    sc.velocity     = velocity;
    sc.fuel         = fuel;
    sc.position.x_au = 1.000f; /* Earth orbit */
    sc.position.y_au = 0.000f;

    printf("--- Spacecraft Record ---\n");
    spacecraft_print_status(&sc); /* arrow notation inside this function */
    printf("\n");

    /* --- Command loop -------------------------------------------------- */

    enum MissionPhase current_phase = PREFLIGHT;

    printf("=========================================\n");
    printf("  CALYPSO COMMAND LOOP ACTIVE\n");
    printf("  n=advance phase  s=sensor scan  m=manifest  e=emergency  q=quit\n");
    printf("=========================================\n\n");

    while (1) {
        /*
         * SOLUTION (Challenge 3): exit automatically once the mission has
         * docked, without waiting for a 'q' command. engine_is_halted()
         * must re-read engine_halted from memory on every iteration -- the
         * volatile qualifier on the underlying variable is what guarantees that.
         */
        if (engine_is_halted()) {
            printf("Mission complete -- engine halted, shutting down.\n");
            break;
        }

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
                    /* SOLUTION (Challenge 3): mission complete -- halt the command loop */
                    engine_halt();
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
            // SOLUTION (Challenge 5 stretch): transmit each crew name as a "TX: <name>" comms line
            crew_transmit_names();

            /* second strncat demo: build a multi-part comms line */
            char manifest_line[64] = "TX[";
            strncat(manifest_line, crew_get_name(0), sizeof(manifest_line) - strlen(manifest_line) - 1);
            strncat(manifest_line, "]",               sizeof(manifest_line) - strlen(manifest_line) - 1);
            printf("Comms line              : %s  (len=%zu)\n\n",
                   manifest_line, strlen(manifest_line));
        }

        if (cmd == 's') {
            /* NOTE: readings[2] = 3200.0f is a demonstration value to show the HIGH_WARN path */
            /* NOTE: readings[0]/[1] reuse the same SENSOR_*_FAULT_* constants as the boot-time check above */
            sensor_float_t readings[3] = { cabin_pressure, velocity, 3200.0f };
            bool faults[3] = {
                sensors_in_fault(readings[0], SENSOR_PRESSURE_FAULT_LOW, SENSOR_PRESSURE_FAULT_HIGH),
                sensors_in_fault(readings[1], SENSOR_VELOCITY_FAULT_LOW, SENSOR_VELOCITY_FAULT_HIGH),
                sensors_in_fault(readings[2], 50.0f, 10000.0f)
            };

#ifdef DEBUG_TELEMETRY
            /* DEBUG_TELEMETRY: raw readings before fault classification -- compiled out unless defined */
            printf("  [DEBUG] raw readings: pressure=%.3f velocity=%.3f spike=%.3f\n",
                   readings[0], readings[1], readings[2]);
#endif
            const int SENSOR_COUNT = 3;

            printf("--- Periodic Sensor Scan ---\n");
            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (faults[i]) {
                    printf("  Sensor %d: FAULTED -- skipping\n", i);
                    /* NOTE: this is the entry log_scan_for_anomaly() finds on the *next* run --
                       it is built with snprintf and handed to log_write(), which still does the
                       actual fprintf() to disk. */
                    char anomaly[64];
                    snprintf(anomaly, sizeof(anomaly), "ANOMALY: sensor %d FAULT during periodic scan", i);
                    log_write(anomaly);
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

    /*
     * Persistent flight log: snapshot mission state into a checkpoint_t and
     * write it with log_save_checkpoint() before the roster it was read from
     * is freed below -- crew_count() must run before crew_free() resets it to 0.
     */
    checkpoint_t final_checkpoint;
    final_checkpoint.mission_phase = (int)current_phase;
    final_checkpoint.fuel_kg       = fuel;
    final_checkpoint.velocity_kms  = velocity;
    final_checkpoint.crew_count    = crew_count();
    // NOTE: checked the same way log_open()'s return value is checked at boot -- both report a failure instead of leaving it to chance.
    if (!log_save_checkpoint("calypso.chk", &final_checkpoint)) {
        printf("  Warning: checkpoint was not saved.\n");
    }
    log_write("MISSION END: normal quit");
    log_close();

    /* every allocation has exactly one matching free */
    free(log_buf); log_buf = NULL;
    crew_free();
    return 0; // NOTE: normal-quit path -- execution does not reach emergency_shutdown label below

emergency_shutdown:
    engine_reset();
    printf("\n--- EMERGENCY SHUTDOWN ---\n");
    printf("ENGINE_CTRL cleared    : 0x%08" PRIX32 "\n", engine_get_ctrl());
    printf("ENGINE_STATUS          : 0x%08" PRIX32 "\n", engine_get_status());
    printf("Calypso offline.\n");

    checkpoint_t shutdown_checkpoint;
    shutdown_checkpoint.mission_phase = (int)current_phase;
    shutdown_checkpoint.fuel_kg       = fuel;
    shutdown_checkpoint.velocity_kms  = velocity;
    shutdown_checkpoint.crew_count    = crew_count();
    if (!log_save_checkpoint("calypso.chk", &shutdown_checkpoint)) {
        printf("  Warning: checkpoint was not saved.\n");
    }
    log_write("MISSION END: emergency shutdown");
    log_close();

    free(log_buf); log_buf = NULL;
    crew_free();
    return 0;
}
