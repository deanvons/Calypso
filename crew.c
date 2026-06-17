/* crew.c -- crew manifest: struct-based roster replacing Phase 11 parallel arrays */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "crew.h"

/*
 * Single struct array replaces three parallel arrays from Phase 11.
 * crew[i] holds every field for one crew member at one index.
 * Index drift -- updating names[i] without ranks[i] -- is now impossible.
 */
static crew_member_t crew[MAX_CREW];
static int           loaded = 0;

static const char *rank_label(CrewRank rank) {
    switch (rank) {
        case RANK_COMMANDER: return "CDR";
        case RANK_PILOT:     return "PLT";
        case RANK_ENGINEER:  return "ENG";
        case RANK_SCIENTIST: return "SCI";
        case RANK_MEDIC:     return "MED";
        default:             return "???";
    }
}

static const char *assignment_label(CrewAssignment a) {
    switch (a) {
        case ASSIGN_FLIGHT:      return "FLIGHT";
        case ASSIGN_ENGINEERING: return "ENG";
        case ASSIGN_SCIENCE:     return "SCI";
        case ASSIGN_MEDICAL:     return "MED";
        default:                 return "???";
    }
}

void crew_init(void) {
    for (int i = 0; i < MAX_CREW; i++) {
        /*
         * Dot notation: crew[i].name is the char array field inside the struct.
         * Same strncpy + explicit null-termination pattern as Phase 11 -- the
         * field is still a char array; the struct wrapper does not change how
         * string functions work on it.
         */
        strncpy(crew[i].name, "UNKNOWN", MAX_NAME_LEN - 1);
        crew[i].name[MAX_NAME_LEN - 1] = '\0';
        crew[i].rank       = RANK_ENGINEER;
        crew[i].id         = 0;
        crew[i].assignment = ASSIGN_FLIGHT;
    }
    loaded = 0;
}

void crew_set_name(int idx, const char *src) {
    if (idx < 0 || idx >= MAX_CREW) return;
    strncpy(crew[idx].name, src, MAX_NAME_LEN - 1);
    crew[idx].name[MAX_NAME_LEN - 1] = '\0';
    if (idx >= loaded) loaded = idx + 1;
}

void crew_set_id(int idx, uint8_t id) {
    if (idx < 0 || idx >= MAX_CREW) return;
    crew[idx].id = id;
}

void crew_set_rank(int idx, CrewRank rank) {
    if (idx < 0 || idx >= MAX_CREW) return;
    crew[idx].rank = rank;
}

const char *crew_get_name(int idx) {
    if (idx < 0 || idx >= MAX_CREW) return "UNKNOWN";
    return crew[idx].name;
}

crew_member_t crew_get_member(int idx) {
    if (idx < 0 || idx >= MAX_CREW) {
        /* return a blank sentinel rather than crashing -- caller gets a safe copy */
        crew_member_t blank;
        strncpy(blank.name, "INVALID", MAX_NAME_LEN - 1);
        blank.name[MAX_NAME_LEN - 1] = '\0';
        blank.rank       = RANK_ENGINEER;
        blank.id         = 0;
        blank.assignment = ASSIGN_FLIGHT;
        return blank;
    }
    return crew[idx]; /* struct copy: all fields copied by value */
}

crew_member_t *crew_get_member_ptr(int idx) {
    if (idx < 0 || idx >= MAX_CREW) return NULL;
    return &crew[idx]; /* address of the live slot -- caller can modify through it */
}

int crew_find_by_name(const char *name) {
    for (int i = 0; i < loaded; i++) {
        /* crew[i].name: dot notation on the array element to reach the name field */
        if (strcmp(crew[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* SOLUTION (Challenge 4): same walk as crew_find_by_name, different field */
int crew_find_by_id(uint8_t id) {
    for (int i = 0; i < loaded; i++) {
        if (crew[i].id == id) {
            return i;
        }
    }
    return -1;
}

int crew_count(void) {
    return loaded;
}

void crew_print_manifest(void) {
    printf("--- Crew Manifest (%d / %d slots) ---\n", loaded, MAX_CREW);
    for (int i = 0; i < loaded; i++) {
        size_t name_len = strlen(crew[i].name);
        printf("  [%03u] %-5s  %-8s  %-*s  (%zu chars)\n",
               (unsigned)crew[i].id,
               rank_label(crew[i].rank),
               assignment_label(crew[i].assignment),
               MAX_NAME_LEN - 1, crew[i].name,
               name_len);
    }
}

void crew_print_member(crew_member_t m) {
    /*
     * m is a copy -- this function received the struct by value.
     * Any change to m here (e.g. m.rank = RANK_COMMANDER) affects only
     * this local copy and is discarded when the function returns.
     * The caller's original record is untouched.
     */
    printf("  [%03u] %-5s  %-8s  %s\n",
           (unsigned)m.id,
           rank_label(m.rank),
           assignment_label(m.assignment),
           m.name);
}

void crew_transmit_names(void) {
    for (int i = 0; i < loaded; i++) {
        char tx_line[48] = "TX: ";
        strncat(tx_line, crew[i].name, sizeof(tx_line) - strlen(tx_line) - 1);
        printf("  %s  (payload=%zu bytes)\n", tx_line, strlen(crew[i].name));
    }
}

void crew_reassign(crew_member_t *m, CrewAssignment new_assignment) {
    if (m == NULL) return;
    /*
     * Arrow notation: m->assignment dereferences the pointer then accesses
     * the field -- equivalent to (*m).assignment = new_assignment.
     * This writes directly into the caller's struct, so the change persists
     * after the function returns. A by-value parameter would discard it.
     */
    m->assignment = new_assignment;
}

/* SOLUTION (Challenge 5): arrow notation writes through the pointer */
void crew_update_rank(crew_member_t *m, CrewRank new_rank) {
    if (m == NULL) return;
    m->rank = new_rank;
}
