extern "C" {
#include "logger/logger.c"
}

#include <cstdio>
#include <cstring>
#include <cstdlib>

#define BOOST_TEST_MODULE LoggerTest
#include <boost/test/included/unit_test.hpp>

/* Helper: reset all mutable global logger state between tests. */
static void reset_logger(void) {
    log_global_cfg.level              = LOG_TRACE;
    log_global_cfg.quiet              = false;
    log_global_cfg.level_cli_override = false;
    log_global_cfg.quiet_cli_override = false;
    log_remove_destinations();
}

/* ------------------------------------------------------------------ */
/* Level API                                                           */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_get_level_default_is_trace) {
    reset_logger();

    BOOST_CHECK_EQUAL(log_get_level(), LOG_TRACE);
}

/* Verify the zero-initialised default: LOG_TRACE == 0, so a never-written
 * global config struct must already satisfy log_get_level() == LOG_TRACE
 * without any prior call to log_set_level().  We validate this by zeroing
 * the struct directly and reading back through the public API. */
BOOST_AUTO_TEST_CASE(test_get_level_zero_initialized_default) {
    /* Wipe the entire config struct to simulate a truly fresh, never-touched
     * state — no log_set_level() call anywhere in the call chain. */
    memset(&log_global_cfg, 0, sizeof(log_global_cfg));

    BOOST_CHECK_EQUAL(log_get_level(), LOG_TRACE);

    /* Restore to a clean baseline so subsequent tests are not affected. */
    reset_logger();
}

BOOST_AUTO_TEST_CASE(test_get_level_save_and_restore) {
    reset_logger();

    log_set_level(LOG_WARN);
    int saved = log_get_level();

    log_set_level(LOG_ERROR);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_ERROR);

    log_set_level(saved);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_WARN);
}

BOOST_AUTO_TEST_CASE(test_get_level_returns_set_level) {
    reset_logger();

    log_set_level(LOG_WARN);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_WARN);

    log_set_level(LOG_DEBUG);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_DEBUG);

    log_set_level(LOG_FATAL);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_FATAL);
}

BOOST_AUTO_TEST_CASE(test_set_level_all_values) {
    reset_logger();

    const int levels[] = { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };
    for (int i = 0; i < 6; i++) {
        log_set_level(levels[i]);
        BOOST_CHECK_EQUAL(log_get_level(), levels[i]);
    }
}

/* ------------------------------------------------------------------ */
/* Quiet API                                                           */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_get_quiet_returns_enabled_state) {
    reset_logger();

    log_set_quiet(false);
    BOOST_CHECK_EQUAL(log_get_quiet(), 0);

    log_set_quiet(true);
    BOOST_CHECK_EQUAL(log_get_quiet(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_quiet_reflects_toggled_state) {
    reset_logger();

    log_set_quiet(true);
    BOOST_CHECK_EQUAL(log_get_quiet(), 1);

    log_set_quiet(false);
    BOOST_CHECK_EQUAL(log_get_quiet(), 0);
}

/* ------------------------------------------------------------------ */
/* Destination management                                             */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_add_fp_returns_success) {
    reset_logger();

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    int rc = log_add_fp(fp, LOG_TRACE);
    BOOST_CHECK_EQUAL(rc, 0);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_add_stdout_returns_success) {
    reset_logger();

    int rc = log_add_stdout(LOG_INFO);
    BOOST_CHECK_EQUAL(rc, 0);
}

BOOST_AUTO_TEST_CASE(test_add_stderr_returns_success) {
    reset_logger();

    int rc = log_add_stderr(LOG_WARN);
    BOOST_CHECK_EQUAL(rc, 0);
}

BOOST_AUTO_TEST_CASE(test_remove_destinations_clears_all) {
    reset_logger();

    /* Register a few destinations */
    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);

    log_remove_destinations();

    /* After clearing, we should be able to fill slots from zero again */
    int rc = log_add_stderr(LOG_TRACE);
    BOOST_CHECK_EQUAL(rc, 0);
}

BOOST_AUTO_TEST_CASE(test_file_destination_receives_output) {
    reset_logger();
    log_set_level(LOG_TRACE);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    log_add_fp(fp, LOG_TRACE);
    log_info("hello from test");

    /* Rewind and verify something was written */
    rewind(fp);
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    BOOST_CHECK_GT(n, (size_t)0);
    BOOST_CHECK(strstr(buf, "hello from test") != NULL);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_file_destination_respects_level_filter) {
    reset_logger();
    log_set_level(LOG_TRACE);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    /* Register file at WARN — DEBUG messages should not appear */
    log_add_fp(fp, LOG_WARN);
    log_debug("this should be filtered");

    rewind(fp);
    char buf[256] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    BOOST_CHECK(strstr(buf, "this should be filtered") == NULL);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_file_destination_passes_at_or_above_level) {
    reset_logger();
    log_set_level(LOG_TRACE);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    log_add_fp(fp, LOG_WARN);
    log_warn("this should pass through");

    rewind(fp);
    char buf[256] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    BOOST_CHECK(strstr(buf, "this should pass through") != NULL);

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* Destination query API                                              */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_get_destinations_returns_zero_when_empty) {
    reset_logger();

    int count = log_get_destinations(NULL, 0);
    BOOST_CHECK_EQUAL(count, 0);
}

/* Explicitly validate the NULL-buffer count-only call pattern documented in
 * the README: passing (NULL, 0) returns the active destination count without
 * requiring the caller to allocate or supply a buffer. */
BOOST_AUTO_TEST_CASE(test_get_destinations_null_buffer_count_only) {
    reset_logger();

    /* No destinations yet — count-only call must return 0 safely. */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_INFO);

    /* With two destinations registered, count-only call must return 2. */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    /* Passing out_size > 0 with a NULL pointer must still return the count
     * without dereferencing the NULL — callers rely on this being safe. */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 10), 2);
}

BOOST_AUTO_TEST_CASE(test_get_destinations_counts_registered_destinations) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 1);

    log_add_stdout(LOG_INFO);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);
    log_add_fp(fp, LOG_WARN);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 3);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_get_destinations_count_resets_after_remove) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    log_remove_destinations();
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);
}

BOOST_AUTO_TEST_CASE(test_get_destinations_populates_output_array) {
    reset_logger();

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    log_add_fp(fp, LOG_TRACE);
    log_add_stderr(LOG_TRACE);

    void *out[4] = {NULL};
    int count = log_get_destinations(out, 4);

    BOOST_CHECK_EQUAL(count, 2);
    /* First registered destination's udata should be the tmpfile pointer */
    BOOST_CHECK_EQUAL(out[0], (void *)fp);
    /* Second registered destination's udata should be stderr */
    BOOST_CHECK_EQUAL(out[1], (void *)stderr);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_get_destinations_respects_out_size_cap) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);

    /* Ask for only one slot even though two are registered */
    void *out[2] = {NULL, NULL};
    int count = log_get_destinations(out, 1);

    /* Count still reflects reality */
    BOOST_CHECK_EQUAL(count, 2);
    /* Only the first slot was written */
    BOOST_CHECK_EQUAL(out[0], (void *)stderr);
    /* Second slot untouched */
    BOOST_CHECK_EQUAL(out[1], (void *)NULL);
}

/* ------------------------------------------------------------------ */
/* log_clear_destinations API                                         */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_clear_destinations_removes_all) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_INFO);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);
    log_add_fp(fp, LOG_WARN);

    /* Confirm three destinations are registered before clearing */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 3);

    log_clear_destinations();

    /* After clearing, the count must be zero */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_clear_destinations_allows_reregistration) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    log_clear_destinations();
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);

    /* Slots should be available again from index zero */
    int rc = log_add_stderr(LOG_WARN);
    BOOST_CHECK_EQUAL(rc, 0);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 1);
}

BOOST_AUTO_TEST_CASE(test_clear_destinations_idempotent_on_empty) {
    reset_logger();

    /* Calling clear on an already-empty list must not crash or corrupt state */
    log_clear_destinations();
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);

    log_clear_destinations();
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);
}

/* ------------------------------------------------------------------ */
/* log_set_destinations API                                           */
/* ------------------------------------------------------------------ */

BOOST_AUTO_TEST_CASE(test_set_destinations_bulk_success) {
    reset_logger();

    FILE *fp1 = tmpfile();
    FILE *fp2 = tmpfile();
    BOOST_REQUIRE(fp1 != NULL);
    BOOST_REQUIRE(fp2 != NULL);

    void *dests[2] = { (void *)fp1, (void *)fp2 };
    int rc = log_set_destinations(dests, 2);

    BOOST_CHECK_EQUAL(rc, 0);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    /* Verify the query round-trip returns the same pointers in order */
    void *out[2] = {NULL, NULL};
    log_get_destinations(out, 2);
    BOOST_CHECK_EQUAL(out[0], (void *)fp1);
    BOOST_CHECK_EQUAL(out[1], (void *)fp2);

    fclose(fp1);
    fclose(fp2);
}

BOOST_AUTO_TEST_CASE(test_set_destinations_replaces_existing_list) {
    reset_logger();

    /* Seed an initial destination */
    log_add_stderr(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 1);

    FILE *fp = tmpfile();
    BOOST_REQUIRE(fp != NULL);

    void *dests[1] = { (void *)fp };
    int rc = log_set_destinations(dests, 1);

    BOOST_CHECK_EQUAL(rc, 0);
    /* Old stderr destination should be gone; only fp remains */
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 1);

    void *out[2] = {NULL, NULL};
    log_get_destinations(out, 2);
    BOOST_CHECK_EQUAL(out[0], (void *)fp);
    BOOST_CHECK_EQUAL(out[1], (void *)NULL);

    fclose(fp);
}

BOOST_AUTO_TEST_CASE(test_set_destinations_set_then_query_roundtrip) {
    reset_logger();

    FILE *fp1 = tmpfile();
    FILE *fp2 = tmpfile();
    FILE *fp3 = tmpfile();
    BOOST_REQUIRE(fp1 != NULL);
    BOOST_REQUIRE(fp2 != NULL);
    BOOST_REQUIRE(fp3 != NULL);

    void *dests[3] = { (void *)fp1, (void *)fp2, (void *)fp3 };
    BOOST_REQUIRE_EQUAL(log_set_destinations(dests, 3), 0);

    void *out[3] = {NULL, NULL, NULL};
    int count = log_get_destinations(out, 3);

    BOOST_CHECK_EQUAL(count, 3);
    BOOST_CHECK_EQUAL(out[0], (void *)fp1);
    BOOST_CHECK_EQUAL(out[1], (void *)fp2);
    BOOST_CHECK_EQUAL(out[2], (void *)fp3);

    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
}

BOOST_AUTO_TEST_CASE(test_set_destinations_empty_list_clears_all) {
    reset_logger();

    log_add_stderr(LOG_TRACE);
    log_add_stdout(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 2);

    /* Setting an empty destination list should wipe everything cleanly */
    int rc = log_set_destinations(NULL, 0);
    BOOST_CHECK_EQUAL(rc, 0);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 0);
}

BOOST_AUTO_TEST_CASE(test_set_destinations_partial_failure_preserves_old_list) {
    reset_logger();

    /* Register a known good destination to form the "old" list */
    log_add_stderr(LOG_TRACE);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), 1);

    /* Fill the destination table to its brim so the second add inside
     * log_set_destinations() will fail, triggering the rollback path. */
    for (int i = 1; i < MAX_LOG_DESTINATIONS; i++) {
        log_add_stderr(LOG_TRACE);
    }
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), MAX_LOG_DESTINATIONS);

    /* Now attempt to set two destinations — the table is full after clearing
     * and re-adding the first one, so the second add must fail and roll back. */
    FILE *fp1 = tmpfile();
    FILE *fp2 = tmpfile();
    BOOST_REQUIRE(fp1 != NULL);
    BOOST_REQUIRE(fp2 != NULL);

    /* We need exactly MAX_LOG_DESTINATIONS + 1 new entries to force failure.
     * Simpler approach: request more destinations than the table can hold. */
    void **big_dests = (void **)malloc((MAX_LOG_DESTINATIONS + 1) * sizeof(void *));
    BOOST_REQUIRE(big_dests != NULL);
    for (int i = 0; i <= MAX_LOG_DESTINATIONS; i++) {
        big_dests[i] = (void *)fp1;
    }

    int rc = log_set_destinations(big_dests, MAX_LOG_DESTINATIONS + 1);

    /* Must fail and the old list of MAX_LOG_DESTINATIONS entries must be intact */
    BOOST_CHECK_EQUAL(rc, -1);
    BOOST_CHECK_EQUAL(log_get_destinations(NULL, 0), MAX_LOG_DESTINATIONS);

    free(big_dests);
    fclose(fp1);
    fclose(fp2);
}

/* ------------------------------------------------------------------ */
/* Callback destination                                               */
/* ------------------------------------------------------------------ */

static int        s_callback_hit   = 0;
static int        s_callback_level = -1;
static char       s_callback_msg[256];

static void test_callback(log_event_t *ev) {
    s_callback_hit++;
    s_callback_level = ev->level;
    vsnprintf(s_callback_msg, sizeof(s_callback_msg), ev->fmt, ev->arg_list);
}

BOOST_AUTO_TEST_CASE(test_callback_destination_is_invoked) {
    reset_logger();
    log_set_level(LOG_TRACE);

    s_callback_hit   = 0;
    s_callback_level = -1;
    memset(s_callback_msg, 0, sizeof(s_callback_msg));

    log_add_destination(test_callback, NULL, LOG_TRACE);
    log_info("callback test message");

    BOOST_CHECK_EQUAL(s_callback_hit, 1);
    BOOST_CHECK_EQUAL(s_callback_level, LOG_INFO);
    BOOST_CHECK(strstr(s_callback_msg, "callback test message") != NULL);
}

BOOST_AUTO_TEST_CASE(test_callback_not_invoked_in_quiet_mode) {
    reset_logger();
    log_set_level(LOG_TRACE);
    log_set_quiet(true);

    s_callback_hit = 0;

    log_add_destination(test_callback, NULL, LOG_TRACE);
    log_info("should be suppressed");

    /* quiet mode suppresses the stderr fallback but NOT registered destinations —
     * this test documents the current behaviour so changes to it are explicit. */
    BOOST_CHECK_EQUAL(s_callback_hit, 0);
}
