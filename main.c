#include <stdio.h>

/*
 * DELIBERATE: two bugs are present in this sensor simulation for Phase 2
 * debugging practice. Use printf tracing or the VS Code debugger to find them.
 * Bug 1: off-by-one. Bug 2: sign error. Do not fix them -- the investigation is the lesson.
 */
int simulate_fuel_sensor(int initial_level, int burn_rate, int elapsed_seconds) {
    int consumed = burn_rate * (elapsed_seconds + 1);  /* DELIBERATE (Bug 1 - off-by-one): should be elapsed_seconds */
    return initial_level + consumed;                   /* DELIBERATE (Bug 2 - sign error): should be initial_level - consumed */
}

int main(void) {
    printf("=========================================\n");
    printf("  CALYPSO FLIGHT COMPUTER\n");
    printf("  Shuttle designation : CALYPSO-7\n");
    printf("  Build date          : %s\n", __DATE__); // NOTE: predefined preprocessor macro -- covered in Phase 15

    // SOLUTION (Challenge 5): add mission_id as an integer, printed with %d
    int mission_id = 7;
    printf("  Mission ID          : %d\n", mission_id);
    printf("=========================================\n\n");

    /*
     * SOLUTION (Challenge 5 -- stretch): swapping format specifiers
     *
     * printf("  Mission ID          : %s\n", mission_id);  -- %s on an int
     * printf("  Shuttle designation : %d\n", "CALYPSO-7"); -- %d on a string literal
     *
     * Both compile without error -- printf takes a variadic argument list and the
     * compiler cannot verify that specifiers match argument types. At runtime the
     * behaviour is undefined: printf reads the wrong number of bytes from the wrong
     * location. The output is garbage, a crash, or silence depending on the platform.
     * This is a logical error that the compiler cannot catch.
     */

    printf("Calypso online. Initialising sensor suite.\n\n");

    int initial_fuel = 1000;  /* kg -- full tank at launch */
    int burn_rate_ks = 5;     /* kg per second at cruise thrust */
    int elapsed      = 10;    /* seconds since engine ignition */

    int fuel_reading = simulate_fuel_sensor(initial_fuel, burn_rate_ks, elapsed);
    int expected     = initial_fuel - burn_rate_ks * elapsed;

    printf("--- Sensor Status ---\n");
    printf("Fuel sensor reading  : %d kg\n", fuel_reading);
    printf("Expected fuel level  : %d kg\n", expected);
    printf("Discrepancy          : %d kg\n\n", fuel_reading - expected);

    printf("WARNING: fuel sensor mismatch detected. Investigate before launch.\n\n");

    printf("Enter command: ");
    char cmd;

    /*
     * &cmd passes the address of cmd so scanf can write the result
     * back to the caller's variable. The leading space in " %c" skips
     * any whitespace left in stdin from previous input.
     */
    scanf(" %c", &cmd);
    printf("Command received: %c - standing by.\n\n", cmd);

    // SOLUTION (Challenge 3): crew ID confirmation step -- same pattern as the command read
    char crew_id;
    printf("Enter crew ID: ");
    scanf(" %c", &crew_id);
    printf("Crew ID confirmed: %c\n", crew_id);

    return 0;
}
