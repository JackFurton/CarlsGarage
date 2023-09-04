#include "logger.h"

#define MAX_LOG_DESTINATIONS 420

/* holds the function pointer for a custom logging function and its level and output stream(udata) */
typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
} log_dest_t;

/* GLOBAL LOG CONFIG: static- so cannot be instantiated, can always be modified via the variable `log_global_cfg` */
static struct {
    log_dest_t destinations[MAX_LOG_DESTINATIONS];
    void *udata;
    int level;
    bool quiet;
} log_global_cfg;

static const char *level_strings[] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL"
};

/* default stdout logger that's always enabled right now */
static void log_to_stdout(log_event_t *ev) {
    /* will hold our time string */
    char time_buf[16];

    /* strftime fills buff with current time in the format of our choosing and returns a length */
    size_t chars_written = strftime(time_buf, sizeof(time_buf), "%H:%M:%S", ev->time);

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

/* basically identical to the above, can skip */
//# TODO refactor this. no reason to have all the duplicate code with the above, it works very similarly
static void log_to_file(log_event_t *ev) {
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(
            ev->udata, "%s %-5s %s:%d: ",
            buf, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->arg_list);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}


const char* log_level_string(int level) {
    return level_strings[level];
}

/*our global cfg mutators that can be used in calling code to modify level or set quiet mode*/
//TODO JF set_level and set_quiet are good candidates for cli flags
void log_set_level(int level) {
    log_global_cfg.level = level;
}

void log_set_quiet(bool enable) {
    log_global_cfg.quiet = enable;
}

/* this is where we register our logging function */
int log_add_destination(log_LogFn fn, void *udata, int level) {
    for (int i = 0; i < MAX_LOG_DESTINATIONS; i++) {
        if (!log_global_cfg.destinations[i].fn) {
            log_global_cfg.destinations[i] = (log_dest_t) { fn, udata, level };
            return 0;
        }
    }
    return -1;
}

/* register a file as the logging destination, notice how the first arg we pass is a log_to_file function ptr */
int log_add_fp(FILE *fp, int level) {
    return log_add_destination(log_to_file, fp, level);
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
 *
 * @param level: request handle
 * @param file: populated by the macro with __FILE__, the current file
 * @param line: populated by the macro with __LINE__, the line number
 * @param fmt: populated by the macro with __LINE__, the line number
 * @return void
 */
void log_log(int level, const char *file, int line, const char *fmt, ...) {
    log_event_t ev = {
            .fmt   = fmt,
            .file  = file,
            .line  = line,
            .level = level,
    };

    if (!log_global_cfg.quiet && level >= log_global_cfg.level) {
        /* finishes populating ev with time and output stream (udata) */
        init_event(&ev, stderr);

        /* ev.arg_list is uninitialized and needs to be populatd with a variable amount of args
         * the va_start function takes the variadic list (ev.arg_list in our code), and the final named argument
         * in the function declaration.  (notice in log_log() signature above, fmt is the last named parameter before the ...)
         * va_start then knows "everything after const char *fmt (the ...) is an arg, so it can take those args
         * and populate (ev.arg_list), our arg list */
        va_start(ev.arg_list, fmt);
        /* now that the struct (ev) has all relevant log info, pass to our concrete std logger function */
        log_to_stdout(&ev);
        va_end(ev.arg_list);
    }

    /* iterate through all the destinations/callbacks and emit the logline there */
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