#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

/* this holds all the context necessary to generate a log line */
typedef struct {
    va_list     arg_list;  /* this is the args list, used to capture a variable number of additional args in your logline */
    const char  *fmt;      /* the basic log string, such as "hello %s" */
    const char  *file;     /* the file the logline came from */
    struct tm   *time;     /* the time the logline was generated */
    void        *udata;    /* output stream, either stderr or file, depending on which log function you use */
    int         line;      /* line number of the logline in the c file */
    int         level;     /* the log severity level of the logline itself, not the system level */
} log_event_t;

/**
 * This is tricky C syntax which defines a function pointer called log_LogFn
 *
 * a function pointer is a variable that points to a function, and thus can hold any function that has the
 * same signature as that defined by the function pointers typedef
 *
 * structure:
 * typedef return_type (*pointer_name)(parameter_types);
 *
 * example:  log_LogFn Foo;
 *
 * Foo must be a pointer to a function that takes the parameter (log_event_t *ev) and returns void
 *
 * Imagine we have some function defined like:
 * void do_stuff(log_event_t *ev) { .. doing stuff here in the function body yall; }
 * we can legally set  Foo = do_stuff, and now Foo is an alias for do_stuff
 * at any time we can call Foo() and it will call do_stuff()
 * later, we could assign a new function to Foo, so it's a flexible way to dynamically change the function we call
 * at runtime based on logic of our choosing
 *
 *
 * The reason we have to wrap (*log_LogFn) in parenthesis is so the compiler knows
 * that we're talking about a type that is a function pointer, which points to function that takes a log_event_t* param and returns void
 *
 * if we instead wrote typedef void *log_LogFn(log_event_t *ev), it would be interpreted as a function that takes a
 * log_event_t* parameter and returns a void*.  this is because the star* binds to whatever is left of it
 */
typedef void (*log_LogFn)(log_event_t *ev);

/* log severity levels */
enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

/* return codes */
enum {
    EXIT_SUCCESS,
    EXIT_FAILURE,
    FILE_OPEN_FAILURE
};


/** These are the main macros that we use to write loglines
 *
 * They invoke our main logging function `log_log` and pass
 * SEVERITY: (LOG_TRACE, LOG_DEBUG etc)
 * __FILE__: built-in preprocessor macro that expands to curr filename
 * __LINE__: built-in preprocessor macro that expands to curr line
 *
 * __VA_ARGS__: capture the variadic args passed when we call the macro
 *
 * For example, when you call `log_trace("woof"), the "woof" string populates VA_ARGS
 * ... syntax is a variadic macro and captures any number of arguments, all of which
 * will expand out into VA_ARGS at call time to be handled by the log_log function
 *
 *
 * Here's an example using our log_debug macro call on line 12
 *
 * log_debug("hello from the underworld %s", x);  will expand at preprocessing to -->
 * log_log(LOG_DEBUG, "logger_test.c", 12, "hello from the underworld %s", x);
 */
#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/* function declarations, all of our function definitions live in the logger.c file */
const char* log_level_string(int level);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);
