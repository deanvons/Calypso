/* crew.c -- crew manifest: dynamic heap-allocated roster (Phase 13) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "crew.h"

/*
 * INITIAL_CAP is deliberately small so that adding the third standard crew
 * member triggers a realloc during startup -- making the growth visible
 * without requiring a separate demo scenario.
 */
#define INITIAL_CAP 2

/*
 * The roster is now a heap pointer, not a fixed-size array.
 * 'capacity' tracks how many crew_member_t slots are allocated on the heap;
 * 'loaded' tracks how many are actually populated.
 */
static crew_member_t *roster   = NULL;
static int            loaded   = 0;
static int            capacity = 0;

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
    /*
     * calloc: allocates INITIAL_CAP structs and zeroes all bytes before returning.
     * Every roster slot starts with name[0]=='\0', id==0, rank==0.
     * Unlike malloc, the initial state is deterministic -- no garbage bytes.
     */
    roster = calloc(INITIAL_CAP, sizeof(crew_member_t));
    if (roster == NULL) {
        fprintf(stderr, "crew_init: allocation failed\n");
        exit(1);
    }
    capacity = INITIAL_CAP;
    loaded   = 0;
}

void crew_free(void) {
    free(roster);
    roster   = NULL;  /* set to NULL after free -- prevents dangling pointer use */
    capacity = 0;
    loaded   = 0;
}

int crew_add(const char *name, CrewRank rank, uint8_t id, CrewAssignment assignment) {
    if (loaded == capacity) {
        /*
         * Roster is full -- double the capacity with realloc.
         *
         * Safe pattern: assign to tmp first. If realloc returns NULL, the
         * original roster pointer is still valid and the old data is intact.
         * Writing realloc's return directly back to roster would lose the only
         * pointer to the existing allocation if realloc fails -- a memory leak.
         */
        int new_cap = capacity * 2;
        crew_member_t *tmp = realloc(roster, (size_t)new_cap * sizeof(crew_member_t));
        if (tmp == NULL) {
            fprintf(stderr, "crew_add: realloc failed (capacity=%d)\n", capacity);
            return -1;
        }
        roster   = tmp;
        capacity = new_cap;
    }
    strncpy(roster[loaded].name, name, MAX_NAME_LEN - 1);
    roster[loaded].name[MAX_NAME_LEN - 1] = '\0';
    roster[loaded].rank       = rank;
    roster[loaded].id         = id;
    roster[loaded].assignment = assignment;
    return loaded++;
}

const char *crew_get_name(int idx) {
    if (idx < 0 || idx >= loaded) return "UNKNOWN";
    return roster[idx].name;
}

int crew_count(void) {
    return loaded;
}

int crew_capacity(void) {
    return capacity;
}

crew_member_t crew_get_member(int idx) {
    if (idx < 0 || idx >= loaded) {
        crew_member_t blank;
        strncpy(blank.name, "INVALID", MAX_NAME_LEN - 1);
        blank.name[MAX_NAME_LEN - 1] = '\0';
        blank.rank       = RANK_ENGINEER;
        blank.id         = 0;
        blank.assignment = ASSIGN_FLIGHT;
        return blank;
    }
    return roster[idx]; /* struct copy: all fields copied by value */
}

crew_member_t *crew_get_member_ptr(int idx) {
    if (idx < 0 || idx >= loaded) return NULL;
    return &roster[idx];
}

int crew_find_by_name(const char *name) {
    for (int i = 0; i < loaded; i++) {
        if (strcmp(roster[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* SOLUTION (Challenge 4): same walk as crew_find_by_name, different field */
int crew_find_by_id(uint8_t id) {
    for (int i = 0; i < loaded; i++) {
        if (roster[i].id == id) {
            return i;
        }
    }
    return -1;
}

void crew_print_manifest(void) {
    printf("--- Crew Manifest (%d loaded / %d capacity) ---\n", loaded, capacity);
    for (int i = 0; i < loaded; i++) {
        size_t name_len = strlen(roster[i].name);
        printf("  [%03u] %-5s  %-8s  %-*s  (%zu chars)\n",
               (unsigned)roster[i].id,
               rank_label(roster[i].rank),
               assignment_label(roster[i].assignment),
               MAX_NAME_LEN - 1, roster[i].name,
               name_len);
    }
}

void crew_print_member(crew_member_t m) {
    /*
     * m is a copy -- received by value. Any change to m here affects only
     * this local copy and is discarded when the function returns.
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
        strncat(tx_line, roster[i].name, sizeof(tx_line) - strlen(tx_line) - 1);
        printf("  %s  (payload=%zu bytes)\n", tx_line, strlen(roster[i].name));
    }
}

void crew_reassign(crew_member_t *m, CrewAssignment new_assignment) {
    if (m == NULL) return;
    /* arrow notation: writes directly into the caller's struct */
    m->assignment = new_assignment;
}

/* SOLUTION (Challenge 5): arrow notation writes through the pointer */
void crew_update_rank(crew_member_t *m, CrewRank new_rank) {
    if (m == NULL) return;
    m->rank = new_rank;
}

/* SOLUTION (Challenge 4): realloc the roster down to exactly loaded slots */
void crew_shrink(void) {
    if (loaded == 0 || loaded == capacity) return;
    crew_member_t *tmp = realloc(roster, (size_t)loaded * sizeof(crew_member_t));
    if (tmp == NULL) {
        fprintf(stderr, "crew_shrink: realloc failed\n");
        return;
    }
    roster   = tmp;
    capacity = loaded;
}
