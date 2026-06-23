/* log.c -- persistent flight log: append-mode text log and binary checkpoint */

#include <stdio.h>
#include <string.h>
#include "log.h"

#define LOG_LINE_MAX 256

/*
 * log_fp: the live handle for this session's text log, opened by log_open()
 * and closed by log_close(). NULL whenever no file is open -- every
 * function below checks that before touching the handle.
 */
static FILE *log_fp = NULL;

bool log_open(const char *path) {
    log_fp = fopen(path, "a");
    if (log_fp == NULL) {
        fprintf(stderr, "log_open: failed to open %s for append\n", path);
        return false;
    }
    return true;
}

void log_close(void) {
    if (log_fp == NULL) {
        return;
    }
    if (fclose(log_fp) != 0) {
        fprintf(stderr, "log_close: error closing log file\n");
    }
    log_fp = NULL;
}

void log_write(const char *entry) {
    if (log_fp == NULL) {
        return;
    }
    fprintf(log_fp, "%s\n", entry);
}

void log_write_raw(const char *line) {
    if (log_fp == NULL) {
        return;
    }
    fputs(line, log_fp);
}

void log_scan_for_anomaly(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("  No previous mission log found (first run).\n");
        return;
    }

    char line[LOG_LINE_MAX];
    char last_anomaly[LOG_LINE_MAX] = "";

    /* fgets reads one line at a time, including the trailing '\n'; it
       returns NULL on end-of-file or on a read error -- feof()/ferror()
       below tell us which one actually happened. */
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "FAULT") != NULL || strstr(line, "ANOMALY") != NULL) {
            strncpy(last_anomaly, line, sizeof(last_anomaly) - 1);
            last_anomaly[sizeof(last_anomaly) - 1] = '\0';
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "log_scan_for_anomaly: read error on %s\n", path);
    } else if (feof(fp) && last_anomaly[0] != '\0') {
        printf("  Previous mission anomaly found: %s", last_anomaly);
    } else {
        printf("  No anomaly recorded in previous mission log.\n");
    }

    fclose(fp);
}

bool log_save_checkpoint(const char *path, const checkpoint_t *snapshot) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "log_save_checkpoint: failed to open %s\n", path);
        return false;
    }

    /* fwrite returns the number of complete elements written -- 1 here means
       every byte of *snapshot made it to the file; anything else is a short write. */
    size_t written = fwrite(snapshot, sizeof(*snapshot), 1, fp);
    fclose(fp);

    if (written != 1) {
        fprintf(stderr, "log_save_checkpoint: short write to %s\n", path);
        return false;
    }
    return true;
}

bool log_load_checkpoint(const char *path, checkpoint_t *out) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false; /* no checkpoint yet -- first run */
    }

    size_t read_count = fread(out, sizeof(*out), 1, fp);
    if (read_count != 1) {
        /*
         * DELIBERATE (not exercised by a normal run): this branch only
         * triggers if calypso.chk exists but is shorter than sizeof(checkpoint_t)
         * (feof) -- e.g. truncated by a crash mid-write -- or the platform
         * reports a genuine I/O error (ferror). Both are distinct from "no
         * file at all", which fopen already handled above.
         */
        if (feof(fp)) {
            fprintf(stderr, "log_load_checkpoint: %s is truncated (incomplete checkpoint)\n", path);
        } else if (ferror(fp)) {
            fprintf(stderr, "log_load_checkpoint: read error on %s\n", path);
        }
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}
