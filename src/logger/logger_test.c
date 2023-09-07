#pragma once

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
    FILE *fp = fopen("boost.log", "a");
    if (fp == NULL) return 1;
    log_add_fp(fp, LOG_TRACE);
    log_trace("ye");
    return 0;
}  