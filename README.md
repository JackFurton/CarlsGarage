# CarlsGarage

## Description

CarlsGarage is a lightweight C/C++ utility library focused on practical, low-overhead tooling for systems programming. The centerpiece right now is the `logger` module — a straightforward, zero-dependency logging library that lets you emit timestamped, severity-tagged log lines to `stderr` or any `FILE *` destination you choose.

The `logger` module supports six severity levels (`TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`) and exposes a clean macro-based API so call-site code stays readable. You can register up to 420 custom log destinations (file pointers or function-pointer callbacks), set a minimum log level to filter noise, and suppress all output with quiet mode. You can also query the current quiet state at runtime with `log_get_quiet()`. A companion `throttled.h` header provides a `throttled(seconds, expr)` macro that rate-limits any expression — handy when you want to log inside a hot loop without drowning in output.

### Level query

```c
/* Query the current minimum log level.
 *
 * Returns the log_level_t value most recently set with log_set_level().
 * If log_set_level() has never been called, returns LOG_TRACE (0),
 * which is the zero-initialised default. */
int log_get_level(void);
```

**Example:**

```c
/* Read back the level that was just configured */
log_set_level(LOG_WARN);
int current = log_get_level();          /* current == LOG_WARN */
printf("minimum level: %d\n", current);

/* Temporarily raise the bar, do some work, then restore */
int saved = log_get_level();
log_set_level(LOG_ERROR);
do_noisy_work();
log_set_level(saved);
```

### Destination query

```c
/* Query the number of currently registered log destinations.
 *
 * If out_destinations is non-NULL, it is filled with up to out_size udata
 * pointers (FILE * or callback udata) for the active destinations, in
 * registration order.  Slots beyond the active count are left untouched.
 *
 * Returns the total number of active destinations regardless of
 * out_size, so callers can pass NULL, 0 to get a pure count. */
int log_get_destinations(void **out_destinations, int out_size);
```

**Example:**

```c
/* Count-only call — find out how many destinations are active right now */
int n = log_get_destinations(NULL, 0);
printf("active destinations: %d\n", n);

/* Buffer-retrieval call — collect udata pointers into a fixed-size array.
 * The size cap means we never overrun the buffer even if more destinations
 * are registered than slots we have available. */
log_add_fp(my_logfile, LOG_INFO);
log_add_stderr(LOG_WARN);

void *dest_buf[4];
int count = log_get_destinations(dest_buf, 4); /* count == 2 */
int slots = count < 4 ? count : 4;             /* whichever is smaller */
for (int i = 0; i < slots; i++) {
    printf("destination %d: %p\n", i, dest_buf[i]);
}
```

```c
/* Query the number of currently registered log destinations.
 *
 * If out_destinations is non-NULL, it is filled with up to out_size udata
 * pointers (FILE * or callback udata) for the active destinations, in
 * registration order.  Slots beyond the active count are left untouched.
 *
 * Returns the total number of active destinations regardless of out_size,
 * so callers can pass NULL/0 for a cheap count-only check. */
int log_get_destinations(void **out_destinations, int out_size);
```

**Example:**

```c
/* Count-only call — no buffer needed */
int n = log_get_destinations(NULL, 0);
printf("active destinations: %d\n", n);

/* Retrieve up to 8 udata pointers */
void *dests[8];
int total = log_get_destinations(dests, 8);
for (int i = 0; i < total && i < 8; i++) {
    printf("destination %d udata: %p\n", i, dests[i]);
}

/* Safe even when size < count — returns true total, writes only size slots */
void *first[1];
int count = log_get_destinations(first, 1); /* count may be > 1 */
``` out_size. */
int log_get_destinations(void **out_destinations, int out_size);
```

**Example:**

```c
/* Quick count — pass NULL/0 to skip the output array */
int n = log_get_destinations(NULL, 0);
printf("%d destination(s) registered\n", n);

/* Retrieve the actual udata pointers (typically FILE * values) */
void *dests[16];
int count = log_get_destinations(dests, 16);
for (int i = 0; i < count; i++) {
    printf("destination %d: %p\n", i, dests[i]);
}
```

## Building

You'll need CMake (3.10+), a C/C++ compiler, and the Boost libraries (for the unit-test framework).

```bash
mkdir build
cd build
cmake ..
make
```

The build produces a `logger` static library and a `test_logger` test executable.

## Logger API

### Quiet mode

```c
/* Suppress all log output */
void log_set_quiet(bool enable);

/* Query the current quiet mode state.
 * Returns 1 if quiet mode is enabled (all output suppressed), 0 otherwise. */
int log_get_quiet(void);
```

**Example:**

```c
log_set_quiet(true);
if (log_get_quiet()) {
    /* logging is currently suppressed */
}
log_set_quiet(false);
```

### Log level

```c
/* Set the minimum severity level that will be emitted */
void log_set_level(int level);  /* LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL */

/* Query the current minimum log level.
 * Returns the current threshold constant (e.g. LOG_INFO). */
int log_get_level(void);
```

**Example:**

```c
log_set_level(LOG_WARN);
if (log_get_level() <= LOG_DEBUG) {
    /* debug logging is active — safe to do expensive preprocessing */
}
```

### Allegro 5 (optional — Linux)

The build system will automatically detect [Allegro 5](https://liballegro.org/) and, if found, compile a small `allegro_demo` executable that opens a window and exercises the logger alongside Allegro's event loop. This is the recommended way to verify that the two libraries play nicely together on your system.

**Install Allegro 5 on common Linux distros:**

```bash
# Debian / Ubuntu
sudo apt install liballegro5-dev liballegro-font5-dev liballegro-primitives5-dev

# Arch Linux
sudo pacman -S allegro

# Fedora / RHEL
sudo dnf install allegro5-devel
```

After installing, just re-run `cmake ..` and `make` — CMake will detect the library via `find_package` first, then fall back to `pkg-config` if a CMake config file is not present. You should see a confirmation line in the CMake output:

```
-- Allegro 5 found — building allegro_demo target
```

If Allegro is not installed you will instead see:

```
-- Allegro 5 not found — skipping allegro_demo target.
```

and everything else will build normally. No breakage, no mandatory dependency.

## Basic Usage

Include the logger header, optionally configure a log level and/or a file destination, then use the level macros to write log lines.

```c
#include "logger/logger.h"
#include <stdio.h>

int main(void) {
    /* Only emit INFO and above; silence TRACE/DEBUG */
    log_set_level(LOG_INFO);

    /* Optionally mirror output to a log file in addition to stderr */
    FILE *fp = fopen("app.log", "w");
    if (fp) {
        log_add_fp(fp, LOG_WARN);  /* file only gets WARN and above */
    }

    log_info("Server starting on port %d", 8080);
    log_warn("Config file not found, using defaults");
    log_error("Failed to bind socket: %s", "address already in use");

    if (fp) {
        fclose(fp);
    }
    return 0;
}
```

### Stdout / Stderr Convenience Functions

In addition to `log_add_fp()`, two convenience wrappers let you direct output to the standard streams without holding a `FILE *` yourself:

```c
log_add_stdout(LOG_DEBUG);  /* send DEBUG and above to stdout */
log_add_stderr(LOG_WARN);   /* send WARN and above to stderr  */
```

Both functions accept the same log-level argument as `log_add_fp()` and return `0` on success or `-1` if the destination table is full.

### Resetting Destinations at Runtime

Call `log_remove_destinations()` to clear every registered destination in one shot. This is handy between test cases or when you want to reconfigure where logs go without restarting your program:

```c
log_remove_destinations();        /* clear everything           */
log_add_fp(new_fp, LOG_INFO);     /* register fresh destination */
```

### Default Fallback Behaviour

If you have not registered any destinations, the logger falls back to writing to `stderr` automatically so log output is never silently swallowed. The moment you register at least one destination, the fallback is bypassed and only your registered destinations receive log lines.

### Callback Destinations

Beyond file pointers, the logger lets you register any function that matches the `log_LogFn` signature as a log destination. This is handy when you want to ship log lines somewhere a `FILE *` can't reach — a ring buffer, a network socket, a GUI console, or a remote telemetry service.

The callback signature is:

```c
typedef void (*log_LogFn)(log_event_t *ev);
```

You register a callback (and an optional `udata` pointer that gets threaded through to your function) with `log_add_destination()`:

```c
int log_add_destination(log_LogFn fn, void *udata, int level);
```

- `fn`    — your callback function
- `udata` — arbitrary context pointer; it arrives in `ev->udata` inside your callback (pass `NULL` if you don't need it)
- `level` — minimum severity this destination will receive

Returns `0` on success, `-1` if the destination table is full (it holds up to 420 entries).

**Complete example — writing log lines into a fixed-size ring buffer:**

```c
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>

#define RING_SIZE 64
#define LINE_SIZE 256

typedef struct {
    char  lines[RING_SIZE][LINE_SIZE];
    int   head;   /* next slot to write */
    int   count;  /* total entries stored (capped at RING_SIZE) */
} ring_buf_t;

/* This is our log_LogFn callback. The logger calls it once per log line
 * that meets the registered minimum level. ev->udata is whatever pointer
 * we passed to log_add_destination(). */
static void cb_ring_buffer(log_event_t *ev) {
    ring_buf_t *ring = (ring_buf_t *)ev->udata;

    /* Build a single formatted string from the variadic args. */
    vsnprintf(ring->lines[ring->head], LINE_SIZE, ev->fmt, ev->arg_list);

    ring->head  = (ring->head + 1) % RING_SIZE;
    if (ring->count < RING_SIZE) ring->count++;
}

int main(void) {
    ring_buf_t ring = {0};

    log_set_level(LOG_DEBUG);

    /* Register our callback; it will receive DEBUG and above. */
    log_add_destination(cb_ring_buffer, &ring, LOG_DEBUG);

    log_info("Server starting on port %d", 8080);
    log_warn("Config file not found, using defaults");
    log_error("Failed to bind socket: %s", "address already in use");

    /* Dump whatever ended up in the ring buffer. */
    printf("--- ring buffer contents (%d entries) ---\n", ring.count);
    for (int i = 0; i < ring.count; i++) {
        int slot = (ring.head - ring.count + i + RING_SIZE) % RING_SIZE;
        printf("  [%d] %s\n", i, ring.lines[slot]);
    }
    return 0;
}
```

**Thread-safety note:** The logger itself does not hold a lock when invoking callbacks, so if your application logs from multiple threads you are responsible for protecting any shared state inside your callback (e.g., wrap the ring-buffer writes in a mutex). `ev->arg_list` is valid only for the duration of the callback — do not store the `va_list` and use it later.

---

To rate-limit a noisy log call (e.g., inside a loop), use the `throttled` macro from `logger/throttled.h`:

```c
#include "logger/throttled.h"
#include "logger/logger.h"

for (int i = 0; i < 1000000; i++) {
    /* Emits at most once every 5 seconds, no matter how tight the loop */
    throttled(5, log_debug("loop iteration %d", i));
}
```

## Tests

After building (see above), run the test executable from the `build` directory:

```bash
cd build
./test_logger
```

The test suite uses the Boost Unit Test Framework. A passing run will print a summary showing the number of test cases executed and any failures.

