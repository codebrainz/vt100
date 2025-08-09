# VT100 Parser Library

This project provides a portable, incremental parser for VT100/ANSI escape
sequences. It is suitable for use in terminal emulators, logging tools, or any
application that needs to interpret terminal control codes.

---

## Features

- Incremental, event-driven parsing of VT100/ANSI escape sequences
- No dynamic memory allocation required
- Supports CSI, OSC, DCS, PM, APC, and standard control/printable characters
- Robust handling of buffer/parameter overflows: string buffers (OSC/DCS/PM/APC)
  are truncated if too long, and CSI parameter arrays are limited to 16 entries
  (extra parameters are ignored)
- Simple callback-based API
- Thorough unit test coverage

## Directory Structure

All files are in the project root unless otherwise noted:

- `vt100.c`, `vt100.h` Library source and header
- `vt100_test.c` Unit tests for the parser
- `CMakeLists.txt` CMake build system
- `Doxyfile.in` Doxygen configuration template
- `LICENSE.md` License
- `README.md` This file

## Quick Start

### One-step build, install, and test

You can use the provided `build.sh` script to perform a full clean build, install the library and header, run all tests, and show the installed files:

```sh
./build.sh
```

This script will:

- Remove any previous build directory for a clean build
- Configure CMake with tests and documentation enabled
- Build the library, test binary, and documentation
- Install the static library and header to `build/install` (see below)
- Run the unit tests (output: `All tests passed!` if successful)
- Show the installed files using `tree -lh` (or `find` if `tree` is not available)

### Manual CMake build (advanced)

You can also build manually with CMake. Optional documentation and test targets can be enabled using CMake options:

```sh
# Basic build (library only):
cmake -S . -B build
cmake --build build

# Build with tests and documentation:
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_DOCS=ON
cmake --build build

# Build only tests or docs:
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cmake -S . -B build -DBUILD_DOCS=ON
cmake --build build --target doc
```

Test binary: `build/bin/vt100_test` (not installed)
Static library: `build/lib/libvt100.a`
Documentation: `build/docs/reference/html/index.html`

### Install location

When using `build.sh` or running `cmake --install build`, the following files are installed to `build/install`:

- `build/install/include/vt100.h` (header)
- `build/install/lib/libvt100.a` (static library)

The test binary is not installed.

### Run tests manually

```sh
build/bin/vt100_test
```

## Usage

Include `vt100.h` in your project and link against `libvt100.a`. Initialize a
parser, provide a callback, and feed input data incrementally:

```c
#include "vt100.h"

void my_callback(vt100_parser_t *parser, const vt100_event_t *event, void *user)
{
    // Handle events here
}

vt100_parser_t parser;
vt100_parser_init(&parser, my_callback, NULL);
vt100_parser_feed(&parser, data, len);
```

### Buffer and Parameter Limits

- **CSI parameters:** Only the first 16 parameters are stored; extra parameters
  are ignored.
- **OSC/DCS/PM/APC strings:** If a string exceeds the buffer size (128 bytes
  including null terminator), it is truncated and the event's
  `is_overflow`/`is_overflowed` field is set.

### XTerm Event Usage Example

To handle XTerm mouse and clipboard events, check the event type in your
callback:

```c
void my_callback(vt100_parser_t *parser, const vt100_event_t *event, void *user)
{
    switch (event->type) {
    case VT100_EVENT_XTERM_MOUSE:
        // Access event->data.xterm_mouse fields
        break;
    case VT100_EVENT_XTERM_CLIPBOARD:
        // Access event->data.xterm_clipboard.data
        break;
    // ... handle other event types ...
    }
}
```

See the unit tests in `vt100_test.c` for more usage examples.

## Documentation

To generate API documentation (HTML) with CMake:

```sh
cmake --build build --target doc
# Open build/docs/reference/html/index.html in your browser
```

## Requirements

- Standard C99 compiler (tested with gcc and clang)
- CMake 3.10+ (recommended)
- `make` (optional, for legacy Makefile)
- `clang-format` (optional, for formatting)

## License

This project is released under the [MIT License](LICENSE.md).
