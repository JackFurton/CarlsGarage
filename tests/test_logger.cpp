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
