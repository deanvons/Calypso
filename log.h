/* log.h -- persistent flight log: append-mode text log and binary checkpoint */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdint.h>
#include "sensors.h"   /* for sensor_float_t */

/*
 * checkpoint_t: a binary snapshot of mission state, written with fwrite()
 * and read back with fread(). mission_phase is stored as a plain int, not
 * enum MissionPhase -- that enum is declared in main.c, and log.h has no
 * reason to depend on it. Every field here is a fixed-width type so the
 * on-disk layout does not depend on which platform wrote it.
 */
typedef struct {
    int            mission_phase;
    uint16_t       fuel_kg;
    sensor_float_t velocity_kms;
    int            crew_count;
} checkpoint_t;

/* --- Text log (calypso.log) ------------------------------------------- */

/* log_open: opens path in append mode ("a"). Returns false if fopen fails. */
bool log_open(const char *path);

/* log_close: closes the handle opened by log_open(); checks fclose()'s return value. */
void log_close(void);

/* log_write: appends entry plus a trailing newline using fprintf(). */
void log_write(const char *entry);

/* log_write_raw: appends line verbatim using fputs() -- no implicit newline. */
void log_write_raw(const char *line);

/*
 * log_scan_for_anomaly: opens path for reading ("r") and walks it line by
 * line with fgets(), looking for any line containing "FAULT" or "ANOMALY".
 * Prints the last such line found, or a "no anomaly" message. Safe to call
 * even if path does not exist yet (first run).
 */
void log_scan_for_anomaly(const char *path);

/* --- Binary checkpoint (calypso.chk) ----------------------------------- */

/* log_save_checkpoint: writes *snapshot to path in binary mode ("wb") via fwrite(). */
bool log_save_checkpoint(const char *path, const checkpoint_t *snapshot);

/*
 * log_load_checkpoint: reads one checkpoint_t from path ("rb") via fread().
 * Returns false if the file does not exist (no prior checkpoint) or the
 * read came back short -- feof()/ferror() distinguish a truncated file from
 * an I/O error.
 */
bool log_load_checkpoint(const char *path, checkpoint_t *out);

#endif /* LOG_H */
