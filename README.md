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

- `vt100.c`, `vt100.h` — Library source and header
- `vt100_test.c` — Unit tests for the parser
- `Makefile` — Build system
- `Doxyfile` — Doxygen configuration
- `build/docs/reference/html/` — Generated API documentation (HTML)
- `LICENSE.md`, `README.md` — License and this file

## Quick Start

1. **Build and test:**

   ```sh
   make test
   ```

   This builds the static library (`build/libvt100.a`) and test binary
   (`build/vt100_test`), then runs all unit tests.

2. **Format code (optional):**

   ```sh
   make format
   ```

3. **Generate documentation (optional):**

```sh
make docs
# Open build/docs/reference/html/index.html in your browser
```

## Building

To build the library and run the tests:

```sh
make test
```

This will build the static library (`build/libvt100.a`) and the test binary
(`build/vt100_test`), then run all unit tests.

To reformat all source/header files using `clang-format`:

```sh
make format
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

To generate API documentation (HTML) with Doxygen:

```sh
make docs
# Open build/docs/reference/html/index.html in your browser
```

## Requirements

- Standard C99 compiler (tested with gcc and clang)
- `make` for building
- `clang-format` for formatting (optional)

## Installation

To install the library and header system-wide (default prefix `/usr/local`):

```sh
sudo make install
```

You can override the install prefix:

```sh
sudo make install PREFIX=/opt
```

This installs `libvt100.a` to `$PREFIX/lib` and `vt100.h` to `$PREFIX/include`.

## Uninstallation

To remove the installed library and header:

```sh
sudo make uninstall
```

## License

This project is released under the [MIT License](LICENSE.md).
