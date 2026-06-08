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

    printf("Calypso online. Enter command.\n\n");

    char cmd;
    printf("> ");

    /*
     * &cmd passes the address of cmd so scanf can write the result
     * back to the caller's variable. Without &, scanf receives a copy
     * and the written value is lost. The leading space in " %c" skips
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
