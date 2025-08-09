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

## Building with CMake

This project supports building with CMake (recommended). You can enable
optional documentation and test targets using CMake options.

### Basic build (library only)

```sh
cmake -S . -B build
cmake --build build
```

### Build with tests and documentation

```sh
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_DOCS=ON
cmake --build build
```

### Build only tests or docs

```sh
# Only tests:
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build

# Only docs:
cmake -S . -B build -DBUILD_DOCS=ON
cmake --build build --target doc
```

Test binary: `build/bin/vt100_test`
Static library: `build/lib/libvt100.a`
Documentation: `build/docs/reference/html/index.html`

### Run tests

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
