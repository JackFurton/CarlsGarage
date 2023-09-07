#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <getopt.h>

#define OPT_DONE -1
#define MAX_LOG_DESTINATIONS 420

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
    LOGGER_EXIT_SUCCESS,
    LOGGER_EXIT_FAILURE,
    LOGGER_FILE_OPEN_FAILURE
};

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

typedef void (*log_LogFn)(log_event_t *ev);

/* holds the function pointer for a custom logging function and its level and output stream(udata) */
typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
} log_dest_t;


/* */ 
typedef struct {
    log_dest_t destinations[MAX_LOG_DESTINATIONS];
    void *udata;
    int level;
    bool quiet;
    bool level_cli_override; 
    bool quiet_cli_override;
} LogGlobalCfg;

extern LogGlobalCfg log_global_cfg;

#ifdef __cplusplus
extern "C" {
#endif


#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)



const char* log_level_string(int level);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_destination(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);
void log_log(int level, const char *file, int line, const char *fmt, ...);
int parse_args(int argc, char *argv[]);

#ifdef __cplusplus
}

#endif //cpp


#endif //LOGGER_H