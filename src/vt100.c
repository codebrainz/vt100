#include "vt100.h"
#include <stdio.h>
/*
 * vt100.c - Incremental VT100 parser implementation
 *
 * This file implements a state machine for parsing VT100/ANSI escape
 * sequences. The parser processes input byte-by-byte and emits events
 * for printable characters, control characters, and recognized sequences.
 *
 * State transitions are handled by per-state handler functions. Each
 * handler is responsible for updating the parser state and emitting
 * events as appropriate. See vt100.h for API documentation.
 */
#include "vt100.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Parser states for the VT100 state machine
typedef enum {
    S_GROUND,     // Default state: printable/control chars, ESC starts sequence
    S_ESC,        // After ESC: next byte determines sequence type
    S_CSI_ENTRY,  // After CSI introducer: parse params/intermediates
    S_CSI_PARAM,  // Parsing CSI parameters
    S_CSI_INTER,  // Parsing CSI intermediate bytes
    S_OSC_STRING, // Parsing OSC string (terminated by BEL or ST)
    S_DCS_ENTRY,  // After DCS introducer
    S_DCS_STRING, // Parsing DCS string (terminated by BEL or ST)
    S_PM_STRING,  // Parsing PM string (terminated by BEL or ST)
    S_APC_STRING  // Parsing APC string (terminated by BEL or ST)
} vt100_state_t;

// vt100_parser_t is now a typedef struct in the header

// Forward declarations for static functions
static void handle_ground(vt100_parser_t* p, char ch);
static void handle_esc(vt100_parser_t* p, char ch);
static void handle_csi_entry(vt100_parser_t* p, char ch);
static void handle_csi_param(vt100_parser_t* p, char ch);
static void handle_csi_inter(vt100_parser_t* p, char ch);
static void handle_osc_string(vt100_parser_t* p, char ch);
static void handle_dcs_entry(vt100_parser_t* p, char ch);
static void handle_dcs_string(vt100_parser_t* p, char ch);

static void emit_event(vt100_parser_t* p, vt100_event_t* ev)
{
    if (p->cb)
        p->cb(p, ev, p->user);
}

void vt100_parser_init(vt100_parser_t* p, vt100_event_cb cb, void* user)
{
    memset(p, 0, sizeof(*p));
    p->cb = cb;
    p->user = user;
    vt100_parser_reset(p);
}

void vt100_parser_reset(vt100_parser_t* p)
{
    p->state = S_GROUND;
    memset(&p->csi, 0, sizeof(p->csi));
    memset(&p->osc, 0, sizeof(p->osc));
    memset(&p->dcs, 0, sizeof(p->dcs));
    p->esc_intermediate = 0;
    p->osc_esc_seen = 0;
    p->osc_overflowed = 0;
    p->dcs_esc_seen = 0;
    p->dcs_overflowed = 0;
    p->pm_esc_seen = 0;
    p->pm_overflowed = 0;
    p->apc_esc_seen = 0;
    p->apc_overflowed = 0;
}

// No destroy needed

/*
 * S_GROUND: Default state. Handles printable, control, and sequence introducers.
 * - Printable/control chars are emitted as events.
 * - ESC or C1 codes transition to sequence states.
 */
static void handle_ground(vt100_parser_t* p, char ch)
{
    vt100_event_t ev = { 0 };
    // Handle C1 8-bit codes as sequence introducers
    if ((unsigned char)ch == 0x9B) { // CSI (C1)
        p->state = S_CSI_ENTRY;
        memset(&p->csi, 0, sizeof(p->csi));
        return;
    } else if ((unsigned char)ch == 0x9D) { // OSC (C1)
        p->state = S_OSC_STRING;
        memset(&p->osc, 0, sizeof(p->osc));
        return;
    } else if ((unsigned char)ch == 0x90) { // DCS (C1)
        p->state = S_DCS_ENTRY;
        memset(&p->dcs, 0, sizeof(p->dcs));
        return;
    } else if ((unsigned char)ch == 0x98) { // SOS (not handled)
        // Emit IGNORE for unsupported SOS sequence
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
        return;
    } else if ((unsigned char)ch == 0x9E) { // PM (C1)
        p->state = S_PM_STRING;
        memset(&p->pm, 0, sizeof(p->pm));
        return;
    } else if ((unsigned char)ch == 0x9F) { // APC (C1)
        p->state = S_APC_STRING;
        memset(&p->apc, 0, sizeof(p->apc));
        return;
    } else if (ch == 0x1B) { // ESC
        p->state = S_ESC;
        return;
    } else if ((ch >= 0x20 && ch <= 0x7E) || ch == '\t' || ch == '\r' || ch == '\n') {
        ev.type = VT100_EVENT_PRINTABLE;
        ev.data.printable = ch;
        emit_event(p, &ev);
    } else if (ch < 0x20 || ch == 0x7F) {
        ev.type = VT100_EVENT_CONTROL;
        ev.data.control = ch;
        emit_event(p, &ev);
    } else {
        // Emit IGNORE for truly unhandled input (not overflow/truncation)
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
    }
}

/*
 * S_ESC: After ESC. Determines the type of sequence to parse next.
 * - '[': CSI, ']': OSC, 'P': DCS, '^': PM, '_': APC, etc.
 * - Final byte emits ESC event and returns to ground.
 */
static void handle_esc(vt100_parser_t* p, char ch)
{
    vt100_event_t ev = { 0 };
    if (ch == '[') {
        p->state = S_CSI_ENTRY;
        memset(&p->csi, 0, sizeof(p->csi));
        return;
    } else if (ch == ']') {
        p->state = S_OSC_STRING;
        memset(&p->osc, 0, sizeof(p->osc));
        return;
    } else if (ch == 'P') {
        p->state = S_DCS_ENTRY;
        memset(&p->dcs, 0, sizeof(p->dcs));
        return;
    } else if (ch == '^') {
        p->state = S_PM_STRING;
        memset(&p->pm, 0, sizeof(p->pm));
        return;
    } else if (ch == '_') {
        p->state = S_APC_STRING;
        memset(&p->apc, 0, sizeof(p->apc));
        return;
    } else if (ch >= 0x20 && ch <= 0x2F) {
        p->esc_intermediate = ch;
        return;
    } else if (ch >= 0x30 && ch <= 0x7E) {
        ev.type = VT100_EVENT_ESCAPE;
        ev.data.escape = ch;
        emit_event(p, &ev);
        p->state = S_GROUND;
        return;
    } else {
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
        p->state = S_GROUND;
    }
}

/*
 * S_CSI_ENTRY: After CSI introducer. Handles private/intermediate bytes,
 * parameters, and final command byte. Transitions to S_CSI_PARAM or S_CSI_INTER.
 */
static void handle_csi_entry(vt100_parser_t* p, char ch)
{
    // Private mode char (0x3C-0x3F) or intermediates (0x20-0x2F)
    if (ch >= 0x3C && ch <= 0x3F) {
        p->csi.private_leader = ch;
        p->state = S_CSI_PARAM;
    } else if (ch >= 0x30 && ch <= 0x39) {
        p->state = S_CSI_PARAM;
        handle_csi_param(p, ch);
    } else if (ch >= 0x20 && ch <= 0x2F) {
        p->state = S_CSI_INTER;
        if (p->csi.num_intermediates < VT100_MAX_CSI_INTERMEDIATES) {
            p->csi.intermediates[p->csi.num_intermediates++] = ch;
        }
    } else if (ch >= 0x40 && ch <= 0x7E) {
        p->csi.command = ch;
        vt100_event_t ev = { 0 };
        // XTerm mouse tracking: CSI M (X10) or CSI < ... M/m (SGR)
        if (ch == 'M' && p->csi.num_intermediates == 0 && p->csi.num_params == 0) {
            ev.type = VT100_EVENT_XTERM_MOUSE;
            emit_event(p, &ev);
            p->state = S_GROUND;
            // Skipping of next 3 bytes is handled in vt100_parser_feed
            return;
        } else if ((ch == 'm' || ch == 'M') && p->csi.private_leader == '<') {
            ev.type = VT100_EVENT_XTERM_MOUSE;
            emit_event(p, &ev);
            p->state = S_GROUND;
            return;
        } else if (ch == 't') {
            ev.type = VT100_EVENT_XTERM_WINOP;
            ev.data.xterm_winop.op = p->csi.params[0];
            ev.data.xterm_winop.num_params = p->csi.num_params;
            for (int i = 0; i < p->csi.num_params && i < 4; ++i) {
                ev.data.xterm_winop.params[i] = p->csi.params[i];
            }
        } else {
            ev.type = VT100_EVENT_CSI;
            ev.data.csi = p->csi;
        }
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
        p->state = S_GROUND;
    }
}

/*
 * S_CSI_PARAM: Parsing CSI parameters (digits and semicolons).
 * - Handles multi-digit and multi-parameter CSI sequences.
 * - Transitions to S_CSI_INTER for intermediate bytes, or emits event on final byte.
 */
static void handle_csi_param(vt100_parser_t* p, char ch)
{
    if (ch >= '0' && ch <= '9') {
        // If this is the first digit of a new parameter, ensure num_params >= 1
        if (p->csi.num_params == 0) {
            p->csi.num_params = 1;
            p->csi.params[0] = 0;
        }
        int* n = &p->csi.params[p->csi.num_params - 1];
        *n = (*n) * 10 + (ch - '0');
    } else if (ch == ';') {
        // Start a new parameter
        if (p->csi.num_params < VT100_MAX_CSI_PARAMS) {
            p->csi.params[p->csi.num_params++] = 0;
        }
    } else if (ch >= 0x20 && ch <= 0x2F) {
        p->state = S_CSI_INTER;
        if (p->csi.num_intermediates < VT100_MAX_CSI_INTERMEDIATES) {
            p->csi.intermediates[p->csi.num_intermediates++] = ch;
        }
    } else if (ch >= 0x40 && ch <= 0x7E) {
        // If no digits or semicolons were seen, treat as one param (default 0)
        if (p->csi.num_params == 0) {
            p->csi.num_params = 1;
            p->csi.params[0] = 0;
        }
        p->csi.command = ch;
        vt100_event_t ev = { 0 };
        /*
         * SGR mouse mode: CSI < ... M/m. All standard XTerm mouse tracking variants are supported.
         * If new variants are introduced, update this handler.
         */
        if ((ch == 'M' || ch == 'm') && p->csi.private_leader == '<') {
            ev.type = VT100_EVENT_XTERM_MOUSE;
            emit_event(p, &ev);
            p->state = S_GROUND;
            return;
        }
        /*
         * XTerm window operation: CSI ... t. All standard XTerm window ops are supported.
         * If new variants are introduced, update this handler.
         */
        if (ch == 't') {
            ev.type = VT100_EVENT_XTERM_WINOP;
            ev.data.xterm_winop.op = p->csi.params[0];
            ev.data.xterm_winop.num_params = p->csi.num_params;
            for (int i = 0; i < p->csi.num_params && i < 4; ++i) {
                ev.data.xterm_winop.params[i] = p->csi.params[i];
            }
            emit_event(p, &ev);
            p->state = S_GROUND;
            return;
        }
        ev.type = VT100_EVENT_CSI;
        ev.data.csi = p->csi;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
        p->state = S_GROUND;
    }
}

/*
 * S_CSI_INTER: Parsing CSI intermediate bytes (0x20-0x2F).
 * - Collects intermediate bytes, emits event on final byte.
 */
static void handle_csi_inter(vt100_parser_t* p, char ch)
{
    if (ch >= 0x20 && ch <= 0x2F) {
        if (p->csi.num_intermediates < VT100_MAX_CSI_INTERMEDIATES)
            p->csi.intermediates[p->csi.num_intermediates++] = ch;
    } else if (ch >= 0x40 && ch <= 0x7E) {
        p->csi.command = ch;
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_CSI;
        ev.data.csi = p->csi;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
        p->state = S_GROUND;
    }
}

/*
 * S_OSC_STRING: Parsing OSC string. Terminated by BEL or ST (ESC \\).
 * - Handles overflow and proper termination.
 */
static void handle_osc_string(vt100_parser_t* p, char ch)
{
    // OSC is terminated by BEL or ST (ESC \\)
    /*
     * Only OSC 52 is recognized as a clipboard event (VT100_EVENT_XTERM_CLIPBOARD).
     * If new clipboard-related OSC codes are introduced in the future, add them here.
     */
    int is_clipboard = 0;
    if (p->osc.length >= 3 && p->osc.string[0] == '5' && p->osc.string[1] == '2' && p->osc.string[2] == ';') {
        is_clipboard = 1;
    }
    if (p->osc_overflowed) {
        // Buffer overflow: emit truncated event (already done), ignore input until terminator
        if (ch == 0x07 || (p->osc_esc_seen && ch == '\\')) {
            // Suppress emission of a second event at the terminator
            p->osc_overflowed = 0;
            p->state = S_GROUND;
            p->osc_esc_seen = 0;
        } else if (ch == 0x1B) {
            p->osc_esc_seen = 1;
        } else {
            p->osc_esc_seen = 0;
        }
        return;
    }
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        if (is_clipboard) {
            ev.type = VT100_EVENT_XTERM_CLIPBOARD;
            size_t n = p->osc.length < VT100_MAX_OSC_STRING ? p->osc.length : VT100_MAX_OSC_STRING - 1;
            memcpy(ev.data.xterm_clipboard.data, p->osc.string, n);
            ev.data.xterm_clipboard.data[n] = 0;
            ev.data.xterm_clipboard.length = n;
            ev.data.xterm_clipboard.is_overflow = 0;
        } else {
            ev.type = VT100_EVENT_OSC;
            p->osc.string[p->osc.length] = 0;
            ev.data.osc = p->osc;
            ev.data.osc.length = p->osc.length;
        }
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->osc_esc_seen = 0;
    } else if (ch == 0x1B) {
        p->osc_esc_seen = 1;
    } else if (p->osc_esc_seen && ch == '\\') {
        vt100_event_t ev = { 0 };
        if (is_clipboard) {
            ev.type = VT100_EVENT_XTERM_CLIPBOARD;
            size_t n = p->osc.length < VT100_MAX_OSC_STRING ? p->osc.length : VT100_MAX_OSC_STRING - 1;
            memcpy(ev.data.xterm_clipboard.data, p->osc.string, n);
            ev.data.xterm_clipboard.data[n] = 0;
            ev.data.xterm_clipboard.length = n;
            ev.data.xterm_clipboard.is_overflow = 0;
        } else {
            ev.type = VT100_EVENT_OSC;
            p->osc.string[p->osc.length] = 0;
            ev.data.osc = p->osc;
            ev.data.osc.length = p->osc.length;
        }
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->osc_esc_seen = 0;
    } else {
        p->osc_esc_seen = 0;
        if (p->osc.length < VT100_MAX_OSC_STRING - 1) {
            p->osc.string[p->osc.length++] = ch;
        } else {
            vt100_event_t ev = { 0 };
            if (is_clipboard) {
                ev.type = VT100_EVENT_XTERM_CLIPBOARD;
                size_t n = p->osc.length < VT100_MAX_OSC_STRING ? p->osc.length : VT100_MAX_OSC_STRING - 1;
                memcpy(ev.data.xterm_clipboard.data, p->osc.string, n);
                ev.data.xterm_clipboard.data[n] = 0;
                ev.data.xterm_clipboard.length = n;
                ev.data.xterm_clipboard.is_overflow = 1;
            } else {
                ev.type = VT100_EVENT_OSC;
                p->osc.string[p->osc.length] = 0;
                ev.data.osc = p->osc;
                ev.data.osc.length = p->osc.length;
            }
            emit_event(p, &ev);
            p->osc_overflowed = 1;
            return; // Prevent further events until terminator
        }
    }
}

/*
 * S_DCS_ENTRY: After DCS introducer. Immediately transitions to S_DCS_STRING.
 */
static void handle_dcs_entry(vt100_parser_t* p, char ch)
{
    p->state = S_DCS_STRING;
    p->dcs.length = 0;
    handle_dcs_string(p, ch);
}

/*
 * S_DCS_STRING: Parsing DCS string. Terminated by BEL or ST (ESC \\).
 */
static void handle_dcs_string(vt100_parser_t* p, char ch)
{
    // DCS is terminated by BEL or ST (ESC \\)
    if (p->dcs_overflowed) {
        // Buffer overflow: emit truncated event (already done), ignore input until terminator
        if (ch == 0x07) {
            p->dcs_overflowed = 0;
            p->state = S_GROUND;
            p->dcs_esc_seen = 0;
        } else if (ch == 0x1B) {
            p->dcs_esc_seen = 1;
        } else if (p->dcs_esc_seen && ch == '\\') {
            p->dcs_overflowed = 0;
            p->state = S_GROUND;
            p->dcs_esc_seen = 0;
        } else {
            p->dcs_esc_seen = 0;
        }
        return;
    }
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_DCS;
        p->dcs.string[p->dcs.length] = 0;
        ev.data.dcs = p->dcs;
        ev.data.dcs.length = p->dcs.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->dcs_esc_seen = 0;
    } else if (ch == 0x1B) {
        p->dcs_esc_seen = 1;
    } else if (p->dcs_esc_seen && ch == '\\') {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_DCS;
        p->dcs.string[p->dcs.length] = 0;
        ev.data.dcs = p->dcs;
        ev.data.dcs.length = p->dcs.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->dcs_esc_seen = 0;
    } else {
        p->dcs_esc_seen = 0;
        if (p->dcs.length < VT100_MAX_DCS_STRING - 1) {
            p->dcs.string[p->dcs.length++] = ch;
        } else {
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_DCS;
            p->dcs.string[p->dcs.length] = 0;
            ev.data.dcs = p->dcs;
            ev.data.dcs.length = p->dcs.length;
            emit_event(p, &ev);
            p->dcs_overflowed = 1;
        }
    }
}
/*
 * S_PM_STRING: Parsing PM string. Terminated by BEL or ST (ESC \\).
 */
static void handle_pm_string(vt100_parser_t* p, char ch)
{
    // PM is terminated by BEL or ST (ESC \\)
    if (p->pm_overflowed) {
        // Buffer overflow: emit truncated event (already done), ignore input until terminator
        if (ch == 0x07) {
            p->pm_overflowed = 0;
            p->state = S_GROUND;
            p->pm_esc_seen = 0;
        } else if (ch == 0x1B) {
            p->pm_esc_seen = 1;
        } else if (p->pm_esc_seen && ch == '\\') {
            p->pm_overflowed = 0;
            p->state = S_GROUND;
            p->pm_esc_seen = 0;
        } else {
            p->pm_esc_seen = 0;
        }
        return;
    }
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_PM;
        p->pm.string[p->pm.length] = 0;
        ev.data.pm = p->pm;
        ev.data.pm.length = p->pm.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->pm_esc_seen = 0;
    } else if (ch == 0x1B) {
        p->pm_esc_seen = 1;
    } else if (p->pm_esc_seen && ch == '\\') {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_PM;
        p->pm.string[p->pm.length] = 0;
        ev.data.pm = p->pm;
        ev.data.pm.length = p->pm.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->pm_esc_seen = 0;
    } else {
        p->pm_esc_seen = 0;
        if (p->pm.length < VT100_MAX_PM_STRING - 1) {
            p->pm.string[p->pm.length++] = ch;
        } else {
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_PM;
            p->pm.string[p->pm.length] = 0;
            ev.data.pm = p->pm;
            ev.data.pm.length = p->pm.length;
            emit_event(p, &ev);
            p->pm_overflowed = 1;
        }
    }
}

/*
 * S_APC_STRING: Parsing APC string. Terminated by BEL or ST (ESC \\).
 */
static void handle_apc_string(vt100_parser_t* p, char ch)
{
    // APC is terminated by BEL or ST (ESC \\)
    if (p->apc_overflowed) {
        // Buffer overflow: emit truncated event (already done), ignore input until terminator
        if (ch == 0x07) {
            p->apc_overflowed = 0;
            p->state = S_GROUND;
            p->apc_esc_seen = 0;
        } else if (ch == 0x1B) {
            p->apc_esc_seen = 1;
        } else if (p->apc_esc_seen && ch == '\\') {
            p->apc_overflowed = 0;
            p->state = S_GROUND;
            p->apc_esc_seen = 0;
        } else {
            p->apc_esc_seen = 0;
        }
        return;
    }
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_APC;
        p->apc.string[p->apc.length] = 0;
        ev.data.apc = p->apc;
        ev.data.apc.length = p->apc.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->apc_esc_seen = 0;
    } else if (ch == 0x1B) {
        p->apc_esc_seen = 1;
    } else if (p->apc_esc_seen && ch == '\\') {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_APC;
        p->apc.string[p->apc.length] = 0;
        ev.data.apc = p->apc;
        ev.data.apc.length = p->apc.length;
        emit_event(p, &ev);
        p->state = S_GROUND;
        p->apc_esc_seen = 0;
    } else {
        p->apc_esc_seen = 0;
        if (p->apc.length < VT100_MAX_APC_STRING - 1) {
            p->apc.string[p->apc.length++] = ch;
        } else {
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_APC;
            p->apc.string[p->apc.length] = 0;
            ev.data.apc = p->apc;
            ev.data.apc.length = p->apc.length;
            emit_event(p, &ev);
            p->apc_overflowed = 1;
        }
    }
}

/*
 * Main parser entry: feeds input bytes to the state machine.
 * Each byte is dispatched to the handler for the current state.
 */
void vt100_parser_feed(vt100_parser_t* p, const char* data, size_t len)
{
    size_t i = 0;
    while (i < len) {
        char ch = data[i];
        if (p->state == S_CSI_ENTRY && ch == 'M' && p->csi.num_intermediates == 0 && p->csi.num_params == 0) {
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_XTERM_MOUSE;
            emit_event(p, &ev);
            p->state = S_GROUND;
            i += 4; // 'M' + 3 bytes
            continue;
        }
        switch (p->state) {
        case S_GROUND:
            handle_ground(p, ch);
            break;
        case S_ESC:
            handle_esc(p, ch);
            break;
        case S_CSI_ENTRY:
            handle_csi_entry(p, ch);
            break;
        case S_CSI_PARAM:
            handle_csi_param(p, ch);
            break;
        case S_CSI_INTER:
            handle_csi_inter(p, ch);
            break;
        case S_OSC_STRING:
            handle_osc_string(p, ch);
            break;
        case S_DCS_ENTRY:
            handle_dcs_entry(p, ch);
            break;
        case S_DCS_STRING:
            handle_dcs_string(p, ch);
            break;
        case S_PM_STRING:
            handle_pm_string(p, ch);
            break;
        case S_APC_STRING:
            handle_apc_string(p, ch);
            break;
        }
        i++;
    }
}
