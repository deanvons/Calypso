/* crew.h -- crew manifest: type definitions and function declarations */

#ifndef CREW_H
#define CREW_H

#include <stdint.h>

// NOTE: #define constants -- preprocessor macros are covered in Phase 15
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
 * All fields for one person live in one struct -- no index convention needed.
 */
typedef struct {
    char           name[MAX_NAME_LEN];
    CrewRank       rank;
    uint8_t        id;
    CrewAssignment assignment;
} crew_member_t;

/* --- Lifecycle ------------------------------------------------------------ */

/* crew_init: allocates the heap roster with calloc (zero-initialised). */
void           crew_init(void);

/* crew_free: releases the heap roster and nulls the pointer. */
void           crew_free(void);

/* --- Roster population --------------------------------------------------- */

/*
 * crew_add: append one crew member to the dynamic roster.
 * If the roster is at capacity, realloc doubles the block before inserting.
 * Returns the slot index on success, or -1 on allocation failure.
 */
int            crew_add(const char *name, CrewRank rank, uint8_t id,
                        CrewAssignment assignment);

/* --- Accessors ------------------------------------------------------------ */

const char    *crew_get_name(int idx);
int            crew_count(void);
int            crew_capacity(void);

/*
 * crew_get_member: returns a copy of the struct at idx by value.
 * The caller receives an independent copy; modifying it does not affect the roster.
 */
crew_member_t  crew_get_member(int idx);

/*
 * crew_get_member_ptr: returns a pointer to the live slot in the roster.
 * The caller can modify the struct in place via -> or crew_reassign.
 * Do not store this pointer across a crew_add call -- realloc may move the block.
 */
crew_member_t *crew_get_member_ptr(int idx);

int            crew_find_by_name(const char *name);

/* SOLUTION (Challenge 4): look up a crew member by numeric ID */
int            crew_find_by_id(uint8_t id);

/* --- Output -------------------------------------------------------------- */

void           crew_print_manifest(void);

/*
 * crew_print_member: receives crew_member_t by value -- a copy.
 * Any change to the parameter inside the function does not reach the caller.
 */
void           crew_print_member(crew_member_t m);

void           crew_transmit_names(void);

/* --- In-place modification ----------------------------------------------- */

/*
 * crew_reassign: updates the assignment field through the pointer.
 * Arrow notation (m->assignment) writes directly to the caller's struct.
 */
void           crew_reassign(crew_member_t *m, CrewAssignment new_assignment);

/*
 * SOLUTION (Challenge 5): update the rank field through the pointer.
 * A by-value parameter would discard the change when the function returns.
 */
void           crew_update_rank(crew_member_t *m, CrewRank new_rank);

/* SOLUTION (Challenge 4): shrink the roster to exactly loaded slots, releasing unused capacity. */
void           crew_shrink(void);

#endif /* CREW_H */
