#include <stdio.h>
#include "logger.c"


int main() {
    char *x = "wigwog";

    log_set_level(LOG_DEBUG);

    log_fatal("we out here %s", x);
    log_debug("we out here %s", x);

    //won't see this
    log_trace("you ain't theen me thorti");
    log_set_level(LOG_TRACE);
    log_trace("oh thit he done got damn expothed me");


    /**
    * #TODO JF function to encapsulate this logic
    *
    *  so we can just pass filename and log level to a function to register it
    */
    FILE *log_file = fopen("debug.log", "a");
    if (log_file == NULL) {
        return EXIT_WHAT_IN_THE_GOT_DAMN_FUCK;
    }
    if (log_add_fp(log_file, LOG_DEBUG) != 0) {
        return EXIT_WHAT_IN_THE_GOT_DAMN_FUCK;
    }


    //testing file registration
    log_fatal("dis logline fatal");
    for(int i = 0; i < 10; ++i) log_error("REEP");
    log_debug("dis shit debug");

}


