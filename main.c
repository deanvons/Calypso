#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

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

    // SOLUTION (Challenge 5): second uint32_t sensor using the same two-step pattern
    uint32_t total_burn_s;
    total_burn_s = mission_elapsed_s; /* accumulated engine burn over the mission so far */

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
    printf("Mission elapsed      : %" PRIu32 " s\n", mission_elapsed_s);
    printf("Total burn           : %" PRIu32 " s\n\n", total_burn_s);

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

    /* --- Engine status register --------------------------------- */

    /*
     * Hardware register values are most readable in hexadecimal -- each hex
     * digit corresponds to exactly 4 bits. 0x1F = 0001 1111 in binary = 31
     * decimal. Binary literals (0b00011111) are supported by GCC and Clang
     * as an extension but are not standard C until C23.
     */
    uint8_t engine_status_reg = 0x1F;
    printf("Engine status reg    : 0x%02X  (decimal: %u)\n\n",
           engine_status_reg, engine_status_reg);

    printf("Enter command: ");
    char cmd;
    scanf(" %c", &cmd);
    printf("Command received: %c - standing by.\n\n", cmd);

    char crew_id;
    printf("Enter crew ID: ");
    scanf(" %c", &crew_id);
    printf("Crew ID confirmed: %c\n", crew_id);

    return 0;
}
