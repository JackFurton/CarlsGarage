#include "logger.h"
#include <string.h>
#include <getopt.h>

LogGlobalCfg log_global_cfg;

static const char *level_strings[] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL"
};

/* default logging function */
static void log_to_stream(log_event_t *ev) {
    /* will hold our time string */
    char time_buf[64];

    /* strftime fills buff with current time in the format of our choosing and returns a length */
    size_t chars_written = strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", ev->time);

    /* use the returned length from strftime to place the null terminator */
    time_buf[chars_written] = '\0';


    /* print the initial part of the log entry to the output stream (ev->udata) which is just stderr now.
     *
     * fprintf takes (filestream, format, and then any number of args (...))
     * we're passing: time (from time_buf) | log level | file name | line number
    */
    fprintf((FILE*)ev->udata, "%s %-5s %s:%d: ", time_buf, level_strings[ev->level], ev->file, ev->line);

    /* vfprintf prints the actual log message we're emitting
     * ev->fmt: format string: in log_debug("hello %s", x) it's the "hello %s" portion
     * ev->arg_list:  arguments passed, in log_debug("hello %s", x) it's the x (and anything after x) portion
     * log_info("bla bla %s, %s", x, y). "bla bla %s %s" is your format string, x and y are args
     * it's perfectly fine to just have a format string without args: log_debug("hello world")
    */
    vfprintf((FILE*)ev->udata, ev->fmt, ev->arg_list);

    /* newline after each logline */
    fprintf((FILE*)ev->udata, "\n");

    /* flush the output buffer to ensure that the log entry is written immediately. */
    fflush((FILE*)ev->udata);

}

//so we're here, and we want to set log level to 0
void log_set_level(int level) {

    //this is the key check that determines whether or not our level set works
    //since we change it permanently to true in the getopt when option is 'l'
    //it will never succeed in changing level again if you pass in level via getopt
    if (log_global_cfg.level_cli_override == false) { 
        log_global_cfg.level = level; 
    } 
}

void log_set_quiet(bool enable) {
    if (log_global_cfg.quiet_cli_override == false) { log_global_cfg.quiet = enable; } 
}

/* this is where we register our logging function */
int log_add_destination(log_LogFn fn, void *udata, int level) {
    for (int i = 0; i < MAX_LOG_DESTINATIONS; i++) {
        if (!log_global_cfg.destinations[i].fn) {
            log_global_cfg.destinations[i] = (log_dest_t) { fn, udata, level };
            return 0;
        }
    }
    return -1; // 
}

/* register a file as the logging destination, notice how the first arg we pass is a log_to_stream function ptr */
int log_add_fp(FILE *fp, int level) {
    return log_add_destination(log_to_stream, fp, level);
}

/* populate our log event struct with time and output stream data */
static void init_event(log_event_t *ev, void *udata) {
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}

/**
 * Top-level logging function invoked by our logging macros
 * When you call log_debug(...), log_info(...) etc, this is what does the work
 *
 * @param level: request handle
 * @param file: populated by the macro with __FILE__, the current file
 * @param line: populated by the macro with __LINE__, the line number
 * @param fmt: populated by the macro with __LINE__, the line number
 * @return void
 */
void log_log(int level, const char *file, int line, const char *fmt, ...) {

    /* initialize logging data, we will pass this to log_to_stream once populated */
    log_event_t ev = {
            .fmt   = fmt,
            .file  = file,
            .line  = line,
            .level = level,
    };

    if (!log_global_cfg.quiet && level >= log_global_cfg.level) {
        
        /* finishes populating ev with time and output stream (udata) */
        init_event(&ev, stderr);

        /* sets up the arg list with all of our extra args if any */
        va_start(ev.arg_list, fmt);
        
        /* HERE'S WHERE LOGGING WORK HAPPENS AND STUFF ACTUALLY GETS PRINTED */
        log_to_stream(&ev);
        
        va_end(ev.arg_list);
    }


    /* you can ignore this for now. 
    *  we iterate through all the registered destinations and call their functions */
    for (int i = 0; i < MAX_LOG_DESTINATIONS && log_global_cfg.destinations[i].fn; i++) {
        log_dest_t *log_dest = &log_global_cfg.destinations[i];
        if (level >= log_dest->level) {
            init_event(&ev, log_dest->udata);
            va_start(ev.arg_list, fmt);
            log_dest->fn(&ev);
            va_end(ev.arg_list);
        }
    }
}

int parse_args(int argc, char *argv[]) {
    
    struct option long_options[] = {
        {"level",     required_argument, 0,   'l' },
        {"quiet",     no_argument,       0,   'q' },
        {0,           0,                 0,    0  }
    };

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
            }
            break;

        case 'q':
            log_info("quiet mode override, logs squelched\n");
            log_set_quiet(true);
            log_global_cfg.quiet_cli_override = true;
            break;
        }
    }
    return LOGGER_EXIT_SUCCESS;
}