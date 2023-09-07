#define BOOST_TEST_MODULE LoggerTest
#include <boost/test/included/unit_test.hpp>
#include <stdio.h>
#include "../src/logger/logger.c"

/*

TODO: add debug.log parsing checks

    FILE *fp = fopen("boost.log", "a");
    if (!fp) return EXIT_FAILURE;
    log_add_fp(fp, LOG_TRACE);
}*/


BOOST_AUTO_TEST_SUITE(LoggerSuite)

BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_debug) 
{
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"debug"};
    parse_args(3, fake_argv);

    //make sure log level is correctly set in config
    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_DEBUG);
    //and since we parsed cli args we the override flag should be set to true
    BOOST_CHECK_EQUAL(log_global_cfg.level_cli_override, true);
}

BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_info) 
{
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"info"};
    parse_args(3, fake_argv);

    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_INFO);
    BOOST_CHECK_EQUAL(log_global_cfg.level_cli_override, true);
    
}

BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_error) 
{
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"error"};
    parse_args(3, fake_argv);

    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_ERROR);
    BOOST_CHECK_EQUAL(log_global_cfg.level_cli_override, true);
}






BOOST_AUTO_TEST_SUITE_END()