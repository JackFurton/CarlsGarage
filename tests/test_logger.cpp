extern "C" {
#include "logger/logger.c"
}

#define BOOST_TEST_MODULE LoggerTest
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_logging) {

}

BOOST_AUTO_TEST_CASE(test_get_level_returns_set_level) {
    log_set_level(LOG_WARN);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_WARN);

    log_set_level(LOG_DEBUG);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_DEBUG);

    log_set_level(LOG_FATAL);
    BOOST_CHECK_EQUAL(log_get_level(), LOG_FATAL);
}

BOOST_AUTO_TEST_CASE(test_get_quiet_returns_enabled_state) {
    log_set_quiet(false);
    BOOST_CHECK_EQUAL(log_get_quiet(), 0);

    log_set_quiet(true);
    BOOST_CHECK_EQUAL(log_get_quiet(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_quiet_reflects_toggled_state) {
    log_set_quiet(true);
    BOOST_CHECK_EQUAL(log_get_quiet(), 1);

    log_set_quiet(false);
    BOOST_CHECK_EQUAL(log_get_quiet(), 0);
}
