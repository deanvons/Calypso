/* crew.h -- crew manifest declarations */

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

/* --- Initialisation ------------------------------------------------------ */

/*
 * crew_init -- resets all name slots to "UNKNOWN" using strncpy.
 * Also resets ranks and ids to defaults.
 */
void        crew_init(void);

/* --- Roster population --------------------------------------------------- */

/*
 * crew_set_name -- copies src into the name slot at idx using strncpy.
 * Guarantees null termination by writing '\0' at names[idx][MAX_NAME_LEN-1]
 * regardless of src length. strncpy alone does not guarantee this when
 * src is longer than MAX_NAME_LEN - 1.
 */
void        crew_set_name(int idx, const char *src);
void        crew_set_id(int idx, uint8_t id);
// SOLUTION (Challenge 4): rank setter -- resolves the DELIBERATE marker in crew_init
void        crew_set_rank(int idx, CrewRank rank);

/* --- Queries ------------------------------------------------------------- */

/*
 * crew_get_name -- returns a const char * to the stored name at idx.
 * Use with strncat to build comms transmission buffers.
 */
const char *crew_get_name(int idx);

/*
 * crew_find_by_name -- walks the roster with strcmp; returns the slot index
 * or -1 if no match. strcmp compares byte sequences -- not pointer values.
 */
int         crew_find_by_name(const char *name);
void        crew_print_manifest(void);
int         crew_count(void);
// SOLUTION (Challenge 5 stretch): builds a "TX: <name>" comms line per slot using strncat
void        crew_transmit_names(void);
