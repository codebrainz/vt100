// ...existing code...
#include "../src/vt100.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
// Helper callback for test_xterm_mouse
static vt100_event_t xterm_mouse_events[8];
static int xterm_mouse_count = 0;
static void xterm_mouse_cb(vt100_parser_t* parser, const vt100_event_t* ev, void* user)
{
    (void)parser;
    (void)user;
    if (xterm_mouse_count < 8)
        xterm_mouse_events[xterm_mouse_count] = *ev;
    xterm_mouse_count++;
}
// vt100_test.c - Unit tests for vt100 parser
#include "../src/vt100.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/*
 * vt100_test.c - Unit tests for vt100 parser
 *
 * This file contains a set of unit tests for the incremental VT100 parser.
 * Each test feeds a specific sequence to the parser and checks that the
 * correct event is emitted. The test harness uses a callback to capture
 * the last event and a counter to verify the number of events.
 */

#define FEED(s) vt100_parser_feed(p, s, strlen(s))

static int event_count = 0;
static vt100_event_t last_event;

// Global test state
void reset_state()
{
    event_count = 0;
    memset(&last_event, 0, sizeof(last_event));
}

void test_cb(vt100_parser_t* parser, const vt100_event_t* ev, void* user)
{
    (void)parser;
    (void)user;
    event_count++; // Test callback: records the last event and increments event count
    memcpy(&last_event, ev, sizeof(*ev));
}
void test_csi_intermediate_private()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B[?1;2;3$z");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CSI);
    assert(last_event.data.csi.private_leader == '?');
    assert(last_event.data.csi.command == 'z');
    assert(last_event.data.csi.params[0] == 1);
    assert(last_event.data.csi.params[1] == 2);
    assert(last_event.data.csi.params[2] == 3);
}

void test_csi_c1()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    char seq[] = { (char)0x9B, '3', '1', 'm', 0 };
    FEED(seq);
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CSI);
    assert(last_event.data.csi.command == 'm');
    assert(last_event.data.csi.params[0] == 31);
}

void test_osc_st()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B]1;foo\x1B\\");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_OSC);
    assert(strncmp(last_event.data.osc.string, "1;foo", 5) == 0);
}

void test_osc_overflow()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    char buf[256];
    memset(buf, 'A', sizeof(buf));
    buf[0] = '\x1B';
    buf[1] = ']';
    buf[sizeof(buf) - 2] = '\x07';
    buf[sizeof(buf) - 1] = 0;
    FEED(buf);
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_OSC);
    assert(last_event.data.osc.length == VT100_MAX_OSC_STRING - 1);
}

void test_dcs_st()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1BPbar\x1B\\");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_DCS);
    assert(strncmp(last_event.data.dcs.string, "bar", 3) == 0);
}

void test_pm_apc()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B^pmtext\x1B\\");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_PM);
    assert(strncmp(last_event.data.pm.string, "pmtext", 6) == 0);
    reset_state();
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B_apctext\x1B\\");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_APC);
    assert(strncmp(last_event.data.apc.string, "apctext", 7) == 0);
}

void test_pm_apc_c1()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    char pmseq[] = { (char)0x9E, 'x', 'y', 0x1B, '\\', 0 };
    FEED(pmseq);
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_PM);
    assert(strncmp(last_event.data.pm.string, "xy", 2) == 0);
    reset_state();
    vt100_parser_init(p, test_cb, NULL);
    char apcseq[] = { (char)0x9F, 'z', 0x1B, '\\', 0 };
    FEED(apcseq);
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_APC);
    assert(strncmp(last_event.data.apc.string, "z", 1) == 0);
}

void test_incremental_osc_st()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B]");
    FEED("foo");
    FEED("\x1B");
    FEED("\\");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_OSC);
    assert(strncmp(last_event.data.osc.string, "foo", 3) == 0);
}

void test_printable()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("A");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_PRINTABLE);
    assert(last_event.data.printable == 'A');
}

void test_control()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x07");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CONTROL);
    assert(last_event.data.control == 0x07);
}

void test_escape()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B>");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_ESCAPE);
    assert(last_event.data.escape == '>');
}

void test_csi_simple()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B[31m");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CSI);
    assert(last_event.data.csi.command == 'm');
    assert(last_event.data.csi.params[0] == 31);
}

void test_csi_multi_param()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B[1;2;3A");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CSI);
    assert(last_event.data.csi.command == 'A');
    assert(last_event.data.csi.params[0] == 1);
    assert(last_event.data.csi.params[1] == 2);
    assert(last_event.data.csi.params[2] == 3);
}

void test_osc()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B]0;Hello\x07");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_OSC);
    assert(strncmp(last_event.data.osc.string, "0;Hello", 7) == 0);
}

void test_dcs()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1BPfoo\x07");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_DCS);
    assert(strncmp(last_event.data.dcs.string, "foo", 3) == 0);
}

void test_incremental()
{
    event_count = 0;
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, NULL);
    FEED("\x1B[");
    FEED("3");
    FEED("1");
    FEED("m");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_CSI);
    assert(last_event.data.csi.command == 'm');
    assert(last_event.data.csi.params[0] == 31);
}

// XTerm event tests
void test_xterm_mouse()
{
    // X10 mouse tracking: CSI M!!!
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    xterm_mouse_count = 0;
    vt100_parser_init(p, xterm_mouse_cb, 0);
    FEED("\x1B[M!!!");
    assert(xterm_mouse_count == 1);
    assert(xterm_mouse_events[0].type == VT100_EVENT_XTERM_MOUSE);

    // SGR mouse tracking: CSI <0;10;20M
    reset_state();
    xterm_mouse_count = 0;
    vt100_parser_init(p, xterm_mouse_cb, 0);
    FEED("\x1B[<0;10;20M");
    assert(xterm_mouse_count == 1);
    assert(xterm_mouse_events[0].type == VT100_EVENT_XTERM_MOUSE);
}

void test_xterm_winop()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, 0);
    // CSI 2 t (window op: iconify window)
    FEED("\x1B[2t");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_XTERM_WINOP);
    assert(last_event.data.xterm_winop.op == 2);
}

void test_xterm_clipboard()
{
    reset_state();
    vt100_parser_t parser;
    vt100_parser_t* p = &parser;
    vt100_parser_init(p, test_cb, 0);
    // OSC 52 ; ; base64data BEL
    FEED("\x1B]52;;SGVsbG8gd29ybGQ=\x07");
    assert(event_count == 1);
    assert(last_event.type == VT100_EVENT_XTERM_CLIPBOARD);
    assert(strncmp(last_event.data.xterm_clipboard.data, "52;;SGVsbG8gd29ybGQ=", 20) == 0);
}

int main(void)
{
    test_printable();
    test_control();
    test_escape();
    test_csi_simple();
    test_csi_multi_param();
    test_osc();
    test_dcs();
    test_incremental();
    test_csi_intermediate_private();
    test_csi_c1();
    test_osc_st();
    test_osc_overflow();
    test_dcs_st();
    test_pm_apc();
    test_pm_apc_c1();
    test_incremental_osc_st();
    test_xterm_mouse();
    test_xterm_winop();
    test_xterm_clipboard();
    // Main: run all tests and print summary
    printf("All tests passed!\n");
    return 0;
}
