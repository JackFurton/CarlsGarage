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
BOOST_AUTO_TEST_CASE(test_getopt_parse_log_level) 
{
    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 1 DEBUG");
    char *fake_debugv[] = { (char*)"woof", (char*)"-l", (char*)"debug"};
    int fake_debugc= sizeof(fake_debugv) / sizeof(char*);
    parse_args(fake_debugc, fake_debugv);
    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_DEBUG);

    BOOST_CHECK_EQUAL(log_global_cfg.level_cli_override, true);

    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 2 INFO");
    char *fake_infov[] = { (char*)"woof", (char*)"-l", (char*)"info"};
    int fake_infoc= sizeof(fake_infov) / sizeof(char*);
    parse_args(fake_infoc, fake_infov);

    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_INFO);

    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 3 ERROR");
    char *fake_errorv[] = { (char*)"woof", (char*)"-l", (char*)"error"};
    int fake_errorc= sizeof(fake_errorv) / sizeof(char*);
    parse_args(fake_errorc, fake_errorv);

    BOOST_CHECK_EQUAL(log_global_cfg.level, LOG_ERROR);

    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("STARTING CASE 4 QUIET");
    char *fake_quietv[] = { (char*)"woof", (char*)"-q", (char*)"quiet mode override, logs squelched\n"};
    int fake_quietc= sizeof(fake_quietv) / sizeof(char*);
    parse_args(fake_quietc, fake_quietv);

    BOOST_CHECK_EQUAL(log_set_quiet, no_argument);

}



BOOST_AUTO_TEST_SUITE_END()