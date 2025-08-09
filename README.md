# VT100 Parser Library

This project provides a portable, incremental parser for VT100/ANSI escape
sequences, suitable for use in terminal emulators, logging tools, or any
application that needs to interpret terminal control codes.

## Features

- Incremental, event-driven parsing of VT100/ANSI escape sequences
- No dynamic memory allocation required
- Supports CSI, OSC, DCS, PM, APC, and standard control/printable characters
- Robust handling of buffer/parameter overflows: string buffers (OSC/DCS/PM/APC) are truncated if too long, and CSI parameter arrays are limited to 16 entries (extra parameters are ignored)
- Simple callback-based API
- Thorough unit test coverage

## Directory Structure

- `src/` - Library source code (`vt100.c`, `vt100.h`)
- `tests/` - Unit tests for the parser
- `docs/` - User guide and documentation

## Building

To build the library and run the tests, simply run:

    make test

This will build the static library (`libvt100.a`) and the test binary
(`vt100_test`), then run all unit tests.

To reformat all source/header files using `clang-format`:

    make format

## Usage

Include `vt100.h` in your project and link against `libvt100.a`. Initialize
a parser, provide a callback, and feed input data incrementally:

```c
#include "vt100.h"

void my_callback(vt100_parser_t *parser, const vt100_event_t *event, void *user) {
    // Handle events here
}

vt100_parser_t parser;
vt100_parser_init(&parser, my_callback, NULL);
vt100_parser_feed(&parser, data, len);
```

### Buffer and Parameter Limits

- **CSI parameters:** Only the first 16 parameters are stored; extra parameters are ignored.
- **OSC/DCS/PM/APC strings:** If a string exceeds the buffer size (128 bytes including null terminator), it is truncated and the event's `is_overflow`/`is_overflowed` field is set.

### XTerm Event Usage Example

To handle XTerm mouse and clipboard events, check the event type in your callback:

```c
void my_callback(vt100_parser_t *parser, const vt100_event_t *event, void *user) {
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

See the unit tests in `tests/vt100_test.c` for more usage examples.

## Documentation

To generate API documentation (HTML) with Doxygen:

    make doc

The generated documentation will be in `docs/reference/html/index.html`.

## Requirements

- Standard C99 compiler (tested with gcc and clang)
- `make` for building
- `clang-format` for formatting (optional)

## License

This project is released under the [MIT License](LICENSE.md).
