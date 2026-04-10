#include "logger.h"

#define MAX_LOG_DESTINATIONS 420

/* holds the function pointer for a custom logging function and its level and output stream(udata) */
typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
} log_dest_t;

/* GLOBAL LOG CONFIG: static - meaning it's always there as a var named log_global_cfg, can modify it at any time */
static struct {
    log_dest_t destinations[MAX_LOG_DESTINATIONS];
    void *udata;
    int level;
    bool quiet;
    bool level_cli_override;  // if true, log_set_level will fail, if false, log_set_level will succeed 
    bool quiet_cli_override;
} log_global_cfg;

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
    fprintf(ev->udata, "%s %-5s %s:%d: ", time_buf, level_strings[ev->level], ev->file, ev->line);

    /* vfprintf prints the actual log message we're emitting
     * ev->fmt: format string: in log_debug("hello %s", x) it's the "hello %s" portion
     * ev->arg_list:  arguments passed, in log_debug("hello %s", x) it's the x (and anything after x) portion
     * log_info("bla bla %s, %s", x, y). "bla bla %s %s" is your format string, x and y are args
     * it's perfectly fine to just have a format string without args: log_debug("hello world")
    */
    vfprintf(ev->udata, ev->fmt, ev->arg_list);

    /* newline after each logline */
    fprintf(ev->udata, "\n");

    /* flush the output buffer to ensure that the log entry is written immediately. */
    fflush(ev->udata);

}

//so we're here, and we want to set log level to 0
int log_get_level(void) {
    return log_global_cfg.level;
}

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

/**
 * Returns the current quiet mode state.
 *
 * @return 1 if quiet mode is enabled (all log output suppressed), 0 otherwise.
 */
int log_get_quiet(void) {
    return log_global_cfg.quiet ? 1 : 0;
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

/* convenience: register stdout as a log destination */
int log_add_stdout(int level) {
    return log_add_destination(log_to_stream, stdout, level);
}

/* convenience: register stderr as a log destination */
int log_add_stderr(int level) {
    return log_add_destination(log_to_stream, stderr, level);
}

/**
 * Returns the count of currently registered log destinations.
 *
 * If out_destinations is non-NULL, it will be filled with up to out_size udata
 * pointers (FILE * or callback udata) for the active destinations, in
 * registration order.  Slots beyond the active count are left untouched.
 *
 * @param out_destinations  Caller-supplied array to receive udata pointers, or NULL.
 * @param out_size          Capacity of out_destinations (ignored when NULL).
 * @return                  Total number of active destinations.
 */
int log_get_destinations(void **out_destinations, int out_size) {
    int count = 0;
    for (int i = 0; i < MAX_LOG_DESTINATIONS; i++) {
        if (log_global_cfg.destinations[i].fn) {
            if (out_destinations != NULL && count < out_size) {
                out_destinations[count] = log_global_cfg.destinations[i].udata;
            }
            count++;
        }
    }
    return count;
}

/* remove all registered destinations (useful for test teardown and runtime reconfiguration) */
void log_remove_destinations(void) {
    for (int i = 0; i < MAX_LOG_DESTINATIONS; i++) {
        log_global_cfg.destinations[i] = (log_dest_t){ NULL, NULL, 0 };
    }
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

        /* If no destinations are registered, fall back to stderr so log output
         * is never silently swallowed. */
        bool has_destinations = false;
        for (int i = 0; i < MAX_LOG_DESTINATIONS; i++) {
            if (log_global_cfg.destinations[i].fn) {
                has_destinations = true;
                break;
            }
        }

        if (!has_destinations) {
            init_event(&ev, stderr);
            va_start(ev.arg_list, fmt);
            log_to_stream(&ev);
            va_end(ev.arg_list);
        }
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