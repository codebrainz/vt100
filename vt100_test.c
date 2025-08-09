/*
 * vt100_test.c - Unit tests for vt100 parser
 *
 * This file contains a set of unit tests for the incremental VT100 parser.
 * Each test feeds a specific sequence to the parser and checks that the
 * correct event is emitted. The test harness uses a callback to capture
 * the last event and a counter to verify the number of events.
 */

#include "vt100.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// --- Macros ---
#define FEED(s) vt100_parser_feed(p, s, strlen(s))

// --- Typedefs for test state ---
typedef struct {
    int count;
    vt100_event_t last_event;
} test_event_last_t;

typedef struct {
    vt100_event_t events[8];
    int count;
} test_event_array_t;

typedef struct {
    vt100_event_t events[4];
    int count;
} test_invalid_state_t;

// --- Helper Callbacks ---
void test_cb(vt100_parser_t* parser, const vt100_event_t* ev, void* user)
{
    (void)parser;
    test_event_last_t* state = (test_event_last_t*)user;
    state->count++;
    memcpy(&state->last_event, ev, sizeof(*ev));
}

void xterm_mouse_cb(vt100_parser_t* parser, const vt100_event_t* ev, void* user)
{
    (void)parser;
    test_event_array_t* arr = (test_event_array_t*)user;
    if (arr->count < 8)
        arr->events[arr->count] = *ev;
    arr->count++;
}

void test_invalid_cb(vt100_parser_t* parser, const vt100_event_t* ev, void* user)
{
    (void)parser;
    test_invalid_state_t* s = (test_invalid_state_t*)user;
    if (s->count < 4)
        s->events[s->count] = *ev;
    s->count++;
}

void test_csi_intermediate_private()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B[?1;2;3$z");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.private_leader == '?');
    assert(state.last_event.data.csi.command == 'z');
    assert(state.last_event.data.csi.params[0] == 1);
    assert(state.last_event.data.csi.params[1] == 2);
    assert(state.last_event.data.csi.params[2] == 3);
}

void test_csi_c1()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char seq[] = { (char)0x9B, '3', '1', 'm', 0 };
    FEED(seq);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.command == 'm');
    assert(state.last_event.data.csi.params[0] == 31);
}

void test_osc_st()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B]1;foo\x1B\\");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_OSC);
    assert(strncmp(state.last_event.data.osc.string, "1;foo", 5) == 0);
}

void test_osc_overflow()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char buf[256];
    memset(buf, 'A', sizeof(buf));
    buf[0] = '\x1B';
    buf[1] = ']';
    buf[sizeof(buf) - 2] = '\x07';
    buf[sizeof(buf) - 1] = 0;
    FEED(buf);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_OSC);
    assert(state.last_event.data.osc.length == VT100_MAX_OSC_STRING - 1);
}

void test_dcs_st()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1BPbar\x1B\\");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_DCS);
    assert(strncmp(state.last_event.data.dcs.string, "bar", 3) == 0);
}

void test_pm_apc()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B^pmtext\x1B\\");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_PM);
    assert(strncmp(state.last_event.data.pm.string, "pmtext", 6) == 0);
    state.count = 0;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B_apctext\x1B\\");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_APC);
    assert(strncmp(state.last_event.data.apc.string, "apctext", 7) == 0);
}

void test_pm_apc_c1()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char pmseq[] = { (char)0x9E, 'x', 'y', 0x1B, '\\', 0 };
    FEED(pmseq);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_PM);
    assert(strncmp(state.last_event.data.pm.string, "xy", 2) == 0);
    state.count = 0;
    vt100_parser_init(p, test_cb, &state);
    char apcseq[] = { (char)0x9F, 'z', 0x1B, '\\', 0 };
    FEED(apcseq);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_APC);
    assert(strncmp(state.last_event.data.apc.string, "z", 1) == 0);
}

void test_incremental_osc_st()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B]");
    FEED("foo");
    FEED("\x1B");
    FEED("\\");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_OSC);
    assert(strncmp(state.last_event.data.osc.string, "foo", 3) == 0);
}

void test_printable()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("A");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_PRINTABLE);
    assert(state.last_event.data.printable == 'A');
}

void test_control()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x07");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CONTROL);
    assert(state.last_event.data.control == 0x07);
}

void test_escape()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B>");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_ESCAPE);
    assert(state.last_event.data.escape == '>');
}

void test_csi_simple()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B[31m");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.command == 'm');
    assert(state.last_event.data.csi.params[0] == 31);
}

void test_csi_multi_param()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B[1;2;3A");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.command == 'A');
    assert(state.last_event.data.csi.params[0] == 1);
    assert(state.last_event.data.csi.params[1] == 2);
    assert(state.last_event.data.csi.params[2] == 3);
}

void test_osc()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B]0;Hello\x07");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_OSC);
    assert(strncmp(state.last_event.data.osc.string, "0;Hello", 7) == 0);
}

void test_dcs()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1BPfoo\x07");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_DCS);
    assert(strncmp(state.last_event.data.dcs.string, "foo", 3) == 0);
}

void test_incremental()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B[");
    FEED("3");
    FEED("1");
    FEED("m");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.command == 'm');
    assert(state.last_event.data.csi.params[0] == 31);
}

// XTerm event tests
void test_xterm_mouse()
{
    // X10 mouse tracking: CSI M!!!
    test_event_array_t arr = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, xterm_mouse_cb, &arr);
    FEED("\x1B[M!!!");
    assert(arr.count == 1);
    assert(arr.events[0].type == VT100_EVENT_XTERM_MOUSE);

    arr.count = 0;
    vt100_parser_init(p, xterm_mouse_cb, &arr);
    FEED("\x1B[<0;10;20M");
    assert(arr.count == 1);
    assert(arr.events[0].type == VT100_EVENT_XTERM_MOUSE);
}

void test_xterm_winop()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    // CSI 2 t (window op: iconify window)
    FEED("\x1B[2t");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_XTERM_WINOP);
    assert(state.last_event.data.xterm_winop.op == 2);
}

void test_xterm_clipboard()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    // OSC 52 ; ; base64data BEL
    FEED("\x1B]52;;SGVsbG8gd29ybGQ=\x07");
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_XTERM_CLIPBOARD);
    assert(strncmp(state.last_event.data.xterm_clipboard.data, "52;;SGVsbG8gd29ybGQ=", 20) == 0);
}

void test_osc_max_length()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char buf[VT100_MAX_OSC_STRING + 4];
    buf[0] = '\x1B';
    buf[1] = ']';
    for (int i = 2; i < VT100_MAX_OSC_STRING + 1; ++i)
        buf[i] = 'X';
    buf[VT100_MAX_OSC_STRING + 1] = '\x07';
    buf[VT100_MAX_OSC_STRING + 2] = 0;
    FEED(buf);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_OSC);
    assert(state.last_event.data.osc.length == VT100_MAX_OSC_STRING - 1);
}

void test_dcs_max_length()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char buf[VT100_MAX_DCS_STRING + 4];
    buf[0] = '\x1B';
    buf[1] = 'P';
    for (int i = 2; i < VT100_MAX_DCS_STRING + 1; ++i)
        buf[i] = 'Y';
    buf[VT100_MAX_DCS_STRING + 1] = '\x07';
    buf[VT100_MAX_DCS_STRING + 2] = 0;
    FEED(buf);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_DCS);
    assert(state.last_event.data.dcs.length == VT100_MAX_DCS_STRING - 1);
}

void test_pm_max_length()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char buf[VT100_MAX_PM_STRING + 4];
    buf[0] = '\x1B';
    buf[1] = '^';
    for (int i = 2; i < VT100_MAX_PM_STRING + 1; ++i)
        buf[i] = 'Z';
    buf[VT100_MAX_PM_STRING + 1] = '\x07';
    buf[VT100_MAX_PM_STRING + 2] = 0;
    FEED(buf);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_PM);
    assert(state.last_event.data.pm.length == VT100_MAX_PM_STRING - 1);
}

void test_apc_max_length()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char buf[VT100_MAX_APC_STRING + 4];
    buf[0] = '\x1B';
    buf[1] = '_';
    for (int i = 2; i < VT100_MAX_APC_STRING + 1; ++i)
        buf[i] = 'Q';
    buf[VT100_MAX_APC_STRING + 1] = '\x07';
    buf[VT100_MAX_APC_STRING + 2] = 0;
    FEED(buf);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_APC);
    assert(state.last_event.data.apc.length == VT100_MAX_APC_STRING - 1);
}

void test_csi_param_truncation()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    char seq[128] = "\x1B[";
    for (int i = 0; i < 20; ++i)
        strcat(seq, "1;");
    strcat(seq, "m");
    FEED(seq);
    assert(state.count == 1);
    assert(state.last_event.type == VT100_EVENT_CSI);
    assert(state.last_event.data.csi.num_params == VT100_MAX_CSI_PARAMS);
}

void test_incomplete_escape()
{
    test_event_last_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, &state);
    FEED("\x1B["); // Incomplete CSI
    assert(state.count == 0);
    FEED("31"); // Still incomplete
    assert(state.count == 0);
}

void test_invalid_sequence()
{
    test_invalid_state_t state = { 0 };
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_invalid_cb, &state);
    FEED("\x1B#\x1B$"); // Invalid ESC intermediates
    assert(state.count == 2);
    assert(state.events[0].type == VT100_EVENT_IGNORE);
    assert(state.events[1].type == VT100_EVENT_PRINTABLE);
    assert(state.events[1].data.printable == '$');
}

int main(void)
{
    test_printable();
    test_control();
    test_escape();
    test_csi_simple();
    test_csi_multi_param();
    test_csi_intermediate_private();
    test_csi_c1();
    test_osc();
    test_osc_st();
    test_osc_overflow();
    test_dcs();
    test_dcs_st();
    test_dcs_max_length();
    test_pm_apc();
    test_pm_apc_c1();
    test_pm_max_length();
    test_apc_max_length();
    test_incremental();
    test_incremental_osc_st();
    test_xterm_mouse();
    test_xterm_winop();
    test_xterm_clipboard();
    test_osc_max_length();
    test_csi_param_truncation();
    test_incomplete_escape();
    test_invalid_sequence();
    printf("All tests passed!\n");
    return 0;
}
