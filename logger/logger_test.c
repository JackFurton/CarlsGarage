#include <stdio.h>
#include "logger.c"


int main() {
    //#TODO use getopt to parse flags that alter log_global_cfg variables


    /**
    * #TODO JF function to encapsulate this logic
    *
    *  so we can just pass filename and log level to your function and register it
    */
    //------------------------------------------
    FILE *log_file = fopen("debug.log", "a");
    if (log_file == NULL) {
        log_error("Failed to open file");
        return FILE_OPEN_FAILURE;
    }

    if (log_add_fp(log_file, LOG_DEBUG) != 0) {
        log_error("%s: Error registering file pointer", __FUNCTION__);
        return EXIT_FAILURE;
    }
    log_info("Successfully registered file pointer: %p", log_file);
   //-------------------------------------------

}


