#include <stdio.h>
#include "logger.c"
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "throttled.h" 
#include <string.h>

struct option long_options[] = {
    {"level",     required_argument, 0,   'l' },
    {"quiet",     no_argument,       0,   'q' },
    {0,           0,                 0,    0  }
};

int main(int argc, char *argv[]) {
    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "l:q", long_options, &long_index)) != OPT_DONE) {

        switch (opt) {
        case 'l':  // set log-level case
            if (strcmp(optarg, "debug") == 0) {
                log_set_level(LOG_DEBUG);
            } else if (strcmp(optarg, "info") == 0) {
                log_set_level(LOG_INFO);
            } else if (strcmp(optarg, "warn") == 0) {
                log_set_level(LOG_WARN);
            } else if (strcmp(optarg, "error") == 0) {
                log_set_level(LOG_ERROR);
            } else if (strcmp(optarg, "fatal") == 0) {
                log_set_level(LOG_FATAL);
            } else {
                log_error("TedP Glares: Invalid log level %s\n", optarg);
                return EXIT_FAILURE;
            }
            break;

        case 'q':
            log_info("quiet mode override, logs squelched\n");
            log_set_quiet(true);
            log_global_cfg.quiet_cli_override = true;
            break;

            }
        }
    log_info("shr1k");

    /* ------------------------------------------------------------------ *
     * Tests for log_clear_destinations()                                  *
     * ------------------------------------------------------------------ */

    /* Test 1: clear an empty destination list — idempotency guard.
     * Calling clear when nothing is registered should be a no-op and
     * not blow anything up (no double-free, no counter underflow). */
    log_clear_destinations();
    int count_after_empty_clear = log_get_destinations(NULL, 0);
    if (count_after_empty_clear != 0) {
        log_error("FAIL: clear on empty list — expected 0 destinations, got %d",
                  count_after_empty_clear);
        return EXIT_FAILURE;
    }
    log_info("PASS: log_clear_destinations() on empty list is idempotent");

    /* Test 2: populate the list, clear it, verify count drops to zero. */
    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);
    int count_before_clear = log_get_destinations(NULL, 0);
    if (count_before_clear != 2) {
        log_error("FAIL: pre-clear setup — expected 2 destinations, got %d",
                  count_before_clear);
        return EXIT_FAILURE;
    }
    log_clear_destinations();
    int count_after_clear = log_get_destinations(NULL, 0);
    if (count_after_clear != 0) {
        log_error("FAIL: after clear — expected 0 destinations, got %d",
                  count_after_clear);
        return EXIT_FAILURE;
    }
    log_info("PASS: log_clear_destinations() drops populated list to zero");

    /* Test 3: clear, then re-register new destinations to confirm the list
     * is truly reset and accepts fresh registrations without issues. */
    log_clear_destinations();
    log_add_stderr(LOG_WARN);
    int count_after_rereg = log_get_destinations(NULL, 0);
    if (count_after_rereg != 1) {
        log_error("FAIL: re-register after clear — expected 1 destination, got %d",
                  count_after_rereg);
        return EXIT_FAILURE;
    }
    log_info("PASS: re-registration after log_clear_destinations() works correctly");

    /* leave the destination list clean for any downstream code */
    log_clear_destinations();
}
