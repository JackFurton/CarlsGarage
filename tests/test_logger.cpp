extern "C" {
#include "logger/logger.c"
}

#include <cstdio>
#include <cstring>

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
