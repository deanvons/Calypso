#include <stdio.h>

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
    int burn_rate    = 5;     /* kg per second at cruise thrust */
    int elapsed      = 10;    /* seconds since engine ignition */

    // SOLUTION (Challenge 3): printf trace -- confirm inputs before the calculation
    printf("DEBUG: initial_fuel=%d  burn_rate=%d  elapsed=%d\n", initial_fuel, burn_rate, elapsed);

    /* DELIBERATE (Bug 1 - off-by-one): should be burn_rate * elapsed */
    int consumed     = burn_rate * (elapsed + 1);

    /* DELIBERATE (Bug 2 - sign error): should be initial_fuel - consumed */
    int fuel_reading = initial_fuel + consumed;

    int expected     = initial_fuel - burn_rate * elapsed;

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
