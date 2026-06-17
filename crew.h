/* crew.h -- crew manifest: type definitions and function declarations */

#include <stdint.h>

// NOTE: #define constants -- preprocessor macros are covered in Phase 15
#define MAX_CREW     6
#define MAX_NAME_LEN 24

typedef enum {
    RANK_COMMANDER,
    RANK_PILOT,
    RANK_ENGINEER,
    RANK_SCIENTIST,
    RANK_MEDIC
} CrewRank;

typedef enum {
    ASSIGN_FLIGHT,
    ASSIGN_ENGINEERING,
    ASSIGN_SCIENCE,
    ASSIGN_MEDICAL
} CrewAssignment;

/*
 * crew_member_t: one crew member as a single named type.
 * Replaces three parallel arrays (names / ranks / ids) from Phase 11.
 * All fields for one person live in one struct -- no index convention needed.
 */
typedef struct {
    char           name[MAX_NAME_LEN];
    CrewRank       rank;
    uint8_t        id;
    CrewAssignment assignment;
} crew_member_t;

/* --- Initialisation ------------------------------------------------------ */

void           crew_init(void);

/* --- Roster population --------------------------------------------------- */

void           crew_set_name(int idx, const char *src);
void           crew_set_id(int idx, uint8_t id);
void           crew_set_rank(int idx, CrewRank rank);

/* --- Accessors ------------------------------------------------------------ */

const char    *crew_get_name(int idx);

/*
 * crew_get_member: returns a copy of the struct at idx by value.
 * The caller receives an independent copy; modifying it does not affect the roster.
 * Use this to pass a crew member to a function that does not need to write back.
 */
crew_member_t  crew_get_member(int idx);

/*
 * crew_get_member_ptr: returns a pointer to the live slot in the roster.
 * The caller can modify the struct in place (via -> or crew_reassign).
 * Do not store this pointer beyond the call -- the roster array is static.
 */
crew_member_t *crew_get_member_ptr(int idx);

int            crew_find_by_name(const char *name);

/* SOLUTION (Challenge 4): look up a crew member by numeric ID */
int            crew_find_by_id(uint8_t id);

int            crew_count(void);

/* --- Output -------------------------------------------------------------- */

void           crew_print_manifest(void);

/*
 * crew_print_member: receives crew_member_t by value -- a copy.
 * Demonstrates that the function works on its own copy and cannot modify
 * the caller's record, regardless of what it does to the parameter.
 */
void           crew_print_member(crew_member_t m);

void           crew_transmit_names(void);

/* --- In-place modification ----------------------------------------------- */

/*
 * crew_reassign: updates the assignment field through the pointer.
 * Arrow notation (m->assignment) writes directly to the caller's struct.
 * Passing by pointer is required because a by-value copy would discard the change.
 */
void           crew_reassign(crew_member_t *m, CrewAssignment new_assignment);

/*
 * SOLUTION (Challenge 5): update the rank field through the pointer.
 * Arrow notation writes directly into the caller's struct -- a by-value
 * parameter would discard the change when the function returns.
 */
void           crew_update_rank(crew_member_t *m, CrewRank new_rank);
