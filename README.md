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

