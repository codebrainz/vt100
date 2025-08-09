# VT100 Parser Project Improvement Plan

This checklist outlines suggested improvements and enhancements for the VT100 parser project, based on a recent code and test review.

## 1. Buffer Overflow Handling

- [x] Make buffer/parameter overflow behavior more visible (documented in code and README; warning comments in code)
- [x] Document CSI parameter truncation (max 16 params) in code and README

## 2. Event Emission Consistency

- [x] Ensure all error/overflow cases emit the correct event (with flags set). Code was already correct; added clarifying comments.
- [x] Emit `VT100_EVENT_IGNORE` for all unhandled/ignored sequences for better diagnostics. Code was already correct; added clarifying comments.

## 3. XTerm Event Parsing

- [x] Review and update OSC 52 clipboard handling for future-proofing (other OSC codes?). Added clarifying comments; code is robust.
- [x] Review XTerm mouse/window ops parsing for completeness (all valid variants covered?). Added clarifying comments; code is robust.

## 4. Test Suite Enhancements

- [x] Add tests for maximum-length OSC/DCS/PM/APC strings (at buffer limit)
- [x] Add tests for CSI sequences with >16 parameters (should be truncated)
- [x] Add tests for malformed/incomplete escape sequences (e.g., ESC [ with no final byte)
- [x] Add negative tests for invalid/partial sequences (assert no spurious events)

## 5. Code Style & Maintenance

- [ ] Remove redundant includes (e.g., duplicate `#include "vt100.h"` in `vt100.c`)
- [ ] Remove unused static functions/variables
- [ ] Consider error codes or error state in parser struct for unrecoverable errors

## 6. Documentation

- [ ] Add usage examples for XTerm events in README
- [ ] Document overflow/truncation behavior for all buffers and parameter arrays

## 7. Build System

- [ ] (Optional) Add install/uninstall targets to Makefiles for easier distribution

---

**Instructions:**

- Check off each item as it is completed.
- For each code/test/doc change, ensure all tests pass before moving to the next item.
- Feel free to add new items as issues are discovered or requirements change.
