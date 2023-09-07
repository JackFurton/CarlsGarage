#include <time.h>

#define throttled(SECONDS_BETWEEN_LOG_INVOCATIONS, x) \
    do { \
        static int reps = 0; \
        static time_t last_rep = 0; \
        ++reps; \
        time_t curr_time = time(NULL); \
        if (curr_time - last_rep > SECONDS_BETWEEN_LOG_INVOCATIONS)  { \
            x; \
            last_rep = curr_time; \
            reps = 0; \
        } \
    } while(0)