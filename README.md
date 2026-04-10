# CarlsGarage

## Description

CarlsGarage is a lightweight C/C++ utility library focused on practical, low-overhead tooling for systems programming. The centerpiece right now is the `logger` module — a straightforward, zero-dependency logging library that lets you emit timestamped, severity-tagged log lines to `stderr` or any `FILE *` destination you choose.

The `logger` module supports six severity levels (`TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`) and exposes a clean macro-based API so call-site code stays readable. You can register up to 420 custom log destinations (file pointers or function-pointer callbacks), set a minimum log level to filter noise, and suppress all output with quiet mode. A companion `throttled.h` header provides a `throttled(seconds, expr)` macro that rate-limits any expression — handy when you want to log inside a hot loop without drowning in output.

## Building

You'll need CMake (3.10+), a C/C++ compiler, and the Boost libraries (for the unit-test framework).

```bash
mkdir build
cd build
cmake ..
make
```

The build produces a `logger` static library and a `test_logger` test executable.

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

