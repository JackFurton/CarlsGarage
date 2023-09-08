#include <stdio.h>

extern "C" {
#include "../src/logger/logger.h"
}
#define BOOST_TEST_MODULE LoggerTest
#include <boost/test/included/unit_test.hpp>
/*

/* #TODOS
----------------------------------------------------------------------
1) 
Add tests that make logs using our macros
and evaluate that the loglines are correct

**** you can register a file for logging like this***
    FILE *fp = fopen("debug.log", "a");
    if (!fp) return EXIT_FAILURE;
    log_add_fp(fp, LOG_DEBUG);

a) register a file
b) make some logs
c) parse the logs and validate the format <--- will be a little tricky 
----------------------------------------------------------------------
*/

BOOST_AUTO_TEST_SUITE(LoggerSuite)


BOOST_AUTO_TEST_CASE(test_getopt_parse_set_quiet) 
{

    optind = 1;
    log_global_cfg.level_cli_override = false;
    BOOST_TEST_MESSAGE("TEST: GETOPT SET QUIET");
    char *fake_argv[] = { (char*)"woof", (char*)"-q"};
    int fake_argc= sizeof(fake_argv) / sizeof(char*);
    parse_args(fake_argc, fake_argv);
    
    BOOST_CHECK_EQUAL(log_global_cfg.quiet, true); 
}

BOOST_AUTO_TEST_CASE(test_getopt_set_log_level)
{

std::vector<std::tuple<std::string, log_level>> test_cases = {
    {"trace",       LOG_TRACE},
    {"debug",       LOG_DEBUG},
    {"info",        LOG_INFO},
    {"warn",        LOG_WARN},
    {"error",       LOG_ERROR},
    {"fatal",       LOG_FATAL},
};
    
    /* iterate and test getopt parsing for all log levels */
    for (const auto&[level_str, level_enum] : test_cases) {
        BOOST_TEST_MESSAGE("TEST: GETOPT: " + level_str);
        char *fake_argv[] = {
            (char*)"program", 
            (char*)"-l", 
            (char*)level_str.c_str()};
        char fake_argc = sizeof(fake_argv) / sizeof(char*); 

        parse_args(fake_argc, fake_argv);
        BOOST_CHECK_EQUAL(log_global_cfg.level, level_enum);

        //reset optind and cli override for next iteration
        log_global_cfg.level_cli_override = false;
        optind = 1;
    }

}



BOOST_AUTO_TEST_SUITE_END()