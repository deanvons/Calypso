/* crew.c -- crew manifest: name storage, lookup, and manifest output */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "crew.h"

/*
 * Parallel arrays: index i in each array refers to the same crew member.
 * The coupling is enforced by convention, not by the type system.
 * Phase 12 replaces this with a crew_member_t struct.
 */
static char     names[MAX_CREW][MAX_NAME_LEN];
static CrewRank ranks[MAX_CREW];
static uint8_t  ids[MAX_CREW];
static int      loaded = 0; /* number of slots populated so far */

/* internal helper -- not exposed in crew.h */
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

void crew_init(void) {
    for (int i = 0; i < MAX_CREW; i++) {
        /*
         * strncpy(dest, src, n): copies at most n bytes from src to dest.
         * "UNKNOWN" is 7 chars + '\0' = 8 bytes -- shorter than MAX_NAME_LEN,
         * so strncpy writes the '\0' itself here. We add it explicitly anyway:
         * good habit because strncpy does NOT null-terminate when src is longer
         * than n-1. See crew_set_name for the case where that matters.
         */
        strncpy(names[i], "UNKNOWN", MAX_NAME_LEN - 1);
        names[i][MAX_NAME_LEN - 1] = '\0';
        ranks[i] = RANK_ENGINEER;
        ids[i]   = 0;
    }
    loaded = 0;
}

void crew_set_name(int idx, const char *src) {
    if (idx < 0 || idx >= MAX_CREW) return;
    /*
     * strncpy copies at most MAX_NAME_LEN - 1 bytes from src.
     * If src is exactly MAX_NAME_LEN - 1 chars or longer, strncpy fills the
     * buffer but does NOT write '\0' -- names[idx] would be unterminated.
     * The explicit assignment on the next line closes that gap unconditionally.
     */
    strncpy(names[idx], src, MAX_NAME_LEN - 1);
    names[idx][MAX_NAME_LEN - 1] = '\0';
    if (idx >= loaded) loaded = idx + 1;
}

void crew_set_id(int idx, uint8_t id) {
    if (idx < 0 || idx >= MAX_CREW) return;
    ids[idx] = id;
}

const char *crew_get_name(int idx) {
    if (idx < 0 || idx >= MAX_CREW) return "UNKNOWN";
    return names[idx];
}

int crew_find_by_name(const char *name) {
    for (int i = 0; i < loaded; i++) {
        /*
         * strcmp returns 0 only when both arrays contain identical byte sequences
         * up to and including the first '\0'. It does NOT compare addresses.
         * Using == here would compare pointer values -- almost always wrong.
         */
        if (strcmp(names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

void crew_print_manifest(void) {
    printf("--- Crew Manifest (%d / %d slots) ---\n", loaded, MAX_CREW);
    for (int i = 0; i < loaded; i++) {
        /* strlen counts bytes from names[i] up to but not including '\0' */
        size_t name_len = strlen(names[i]);
        printf("  [%03u] %-5s  %-*s  (%zu chars)\n",
               ids[i], rank_label(ranks[i]),
               MAX_NAME_LEN - 1, names[i],
               name_len);
    }
}

int crew_count(void) {
    return loaded;
}
