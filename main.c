#include <stdio.h>

int main(void) {
    printf("=========================================\n");
    printf("  CALYPSO FLIGHT COMPUTER\n");
    printf("  Shuttle designation : CALYPSO-7\n");
    printf("  Build date          : %s\n", __DATE__); // NOTE: predefined preprocessor macro - covered in phase-15
    printf("=========================================\n\n");

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
    printf("Command received: %c - standing by.\n", cmd);

    return 0;
}
