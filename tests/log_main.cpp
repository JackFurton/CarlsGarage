#include <stdio.h>

extern "C" {
#include "../src/logger/logger.h"
}

#define BOOST_TEST_MODULE LoggerTest
#include <boost/test/included/unit_test.hpp>

/*
TODO: add debug.log parsing checks

    FILE *fp = fopen("boost.log", "a");
    if (!fp) return EXIT_FAILURE;
    log_add_fp(fp, LOG_TRACE);

TODO:     
}*/

BOOST_AUTO_TEST_SUITE(LoggerSuite)



/** Important: getopt maintains global state.  Between tests you need to 
*   reset optind or you will get strange and unexpected behavior.
*/
BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_debug) 
{
    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 1");
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"debug"};
    int fake_argc = sizeof(fake_argv) / sizeof(char*);
    parse_args(fake_argc, fake_argv);
    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_DEBUG);

    /*
        #TODO JF: This test should pass but does not, plox fix
    */
    BOOST_CHECK_EQUAL(log_global_cfg.level_cli_override, true);
    /*
        #TODO JF: when you fix this, its expected that it breaks 
        the asserts in the following tests for log_level checks.
        so try to fix those.

    */

}

BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_info) 
{
    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 2");
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"info"};
    int fake_argc = sizeof(fake_argv) / sizeof(char*);
    parse_args(fake_argc, fake_argv);

    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_INFO);


}

BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level_error) 
{
    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 3");
    char *fake_argv[] = { (char*)"woof", (char*)"-l", (char*)"error"};
    int fake_argc = sizeof(fake_argv) / sizeof(char*);
    parse_args(fake_argc, fake_argv);


    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_ERROR);

}



BOOST_AUTO_TEST_SUITE_END()