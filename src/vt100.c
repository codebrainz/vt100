// vt100.c - Incremental VT100 parser implementation
#include "vt100.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Parser states
typedef enum {
    S_GROUND,
    S_ESC,
    S_CSI_ENTRY,
    S_CSI_PARAM,
    S_CSI_INTER,
    S_OSC_STRING,
    S_DCS_ENTRY,
    S_DCS_STRING,
    S_PM_STRING,
    S_APC_STRING
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
}

// No destroy needed

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
        ev.type = VT100_EVENT_IGNORE;
        emit_event(p, &ev);
    }
}

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
        p->csi.intermediates[0] = ch;
        p->csi.num_intermediates = 1;
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

static void handle_csi_param(vt100_parser_t* p, char ch)
{
    if (ch >= '0' && ch <= '9') {
        int* n = &p->csi.params[p->csi.num_params];
        *n = (*n) * 10 + (ch - '0');
    } else if (ch == ';') {
        if (p->csi.num_params < VT100_MAX_CSI_PARAMS - 1) {
            p->csi.num_params++;
        }
    } else if (ch >= 0x20 && ch <= 0x2F) {
        if (p->csi.num_intermediates < VT100_MAX_CSI_INTERMEDIATES)
            p->csi.intermediates[p->csi.num_intermediates++] = ch;
        p->state = S_CSI_INTER;
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

static void handle_osc_string(vt100_parser_t* p, char ch)
{
    // OSC is terminated by BEL or ST (ESC \\)
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_OSC;
        ev.data.osc = p->osc;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else if (ch == '\\') {
        if (p->osc.length > 0 && p->osc.string[p->osc.length - 1] == 0x1B) {
            p->osc.length--; // Remove ESC
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_OSC;
            ev.data.osc = p->osc;
            emit_event(p, &ev);
            p->state = S_GROUND;
        } else if (p->osc.length < VT100_MAX_OSC_STRING - 1) {
            p->osc.string[p->osc.length++] = ch;
        }
    } else if (p->osc.length < VT100_MAX_OSC_STRING - 1) {
        p->osc.string[p->osc.length++] = ch;
    }
}

static void handle_dcs_entry(vt100_parser_t* p, char ch)
{
    p->state = S_DCS_STRING;
    p->dcs.length = 0;
    handle_dcs_string(p, ch);
}

static void handle_dcs_string(vt100_parser_t* p, char ch)
{
    // DCS is terminated by BEL or ST (ESC \\)
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_DCS;
        ev.data.dcs = p->dcs;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else if (ch == '\\') {
        if (p->dcs.length > 0 && p->dcs.string[p->dcs.length - 1] == 0x1B) {
            p->dcs.length--; // Remove ESC
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_DCS;
            ev.data.dcs = p->dcs;
            emit_event(p, &ev);
            p->state = S_GROUND;
        } else if (p->dcs.length < VT100_MAX_DCS_STRING - 1) {
            p->dcs.string[p->dcs.length++] = ch;
        }
    } else if (p->dcs.length < VT100_MAX_DCS_STRING - 1) {
        p->dcs.string[p->dcs.length++] = ch;
    }
}
static void handle_pm_string(vt100_parser_t* p, char ch)
{
    // PM is terminated by BEL or ST (ESC \\)
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_PM;
        ev.data.pm = p->pm;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else if (ch == '\\') {
        if (p->pm.length > 0 && p->pm.string[p->pm.length - 1] == 0x1B) {
            p->pm.length--;
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_PM;
            ev.data.pm = p->pm;
            emit_event(p, &ev);
            p->state = S_GROUND;
        } else if (p->pm.length < VT100_MAX_PM_STRING - 1) {
            p->pm.string[p->pm.length++] = ch;
        }
    } else if (p->pm.length < VT100_MAX_PM_STRING - 1) {
        p->pm.string[p->pm.length++] = ch;
    }
}

static void handle_apc_string(vt100_parser_t* p, char ch)
{
    // APC is terminated by BEL or ST (ESC \\)
    if (ch == 0x07) {
        vt100_event_t ev = { 0 };
        ev.type = VT100_EVENT_APC;
        ev.data.apc = p->apc;
        emit_event(p, &ev);
        p->state = S_GROUND;
    } else if (ch == '\\') {
        if (p->apc.length > 0 && p->apc.string[p->apc.length - 1] == 0x1B) {
            p->apc.length--;
            vt100_event_t ev = { 0 };
            ev.type = VT100_EVENT_APC;
            ev.data.apc = p->apc;
            emit_event(p, &ev);
            p->state = S_GROUND;
        } else if (p->apc.length < VT100_MAX_APC_STRING - 1) {
            p->apc.string[p->apc.length++] = ch;
        }
    } else if (p->apc.length < VT100_MAX_APC_STRING - 1) {
        p->apc.string[p->apc.length++] = ch;
    }
}

void vt100_parser_feed(vt100_parser_t* p, const char* data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char ch = data[i];
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
    }
}
