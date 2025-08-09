
/* vt100.h - Incremental VT100 parser interface */
#ifndef VT100_H
#define VT100_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// VT100 parser event types
typedef enum {
    VT100_EVENT_NONE = 0,
    VT100_EVENT_PRINTABLE,
    VT100_EVENT_CONTROL,
    VT100_EVENT_ESCAPE,
    VT100_EVENT_CSI,
    VT100_EVENT_OSC,
    VT100_EVENT_DCS,
    VT100_EVENT_PM, // Privacy Message (ESC ^ ... ST)
    VT100_EVENT_APC, // Application Program Command (ESC _ ... ST)
    VT100_EVENT_IGNORE
} vt100_event_type_t;

// CSI parameter limits
#define VT100_MAX_CSI_PARAMS 16
#define VT100_MAX_CSI_INTERMEDIATES 4
#define VT100_MAX_OSC_STRING 128
#define VT100_MAX_DCS_STRING 128
#define VT100_MAX_PM_STRING 128
#define VT100_MAX_APC_STRING 128

// CSI event data
typedef struct
{
    char command; // Final byte (0x40-0x7E)
    int params[VT100_MAX_CSI_PARAMS];
    int num_params;
    char intermediates[VT100_MAX_CSI_INTERMEDIATES];
    int num_intermediates;
    char private_leader; // e.g. '?', '>'
} vt100_csi_t;

// PM event data
typedef struct
{
    char string[VT100_MAX_PM_STRING];
    size_t length;
} vt100_pm_t;

// APC event data
typedef struct
{
    char string[VT100_MAX_APC_STRING];
    size_t length;
} vt100_apc_t;

// OSC event data
typedef struct
{
    char string[VT100_MAX_OSC_STRING];
    size_t length;
} vt100_osc_t;

// DCS event data
typedef struct
{
    char string[VT100_MAX_DCS_STRING];
    size_t length;
} vt100_dcs_t;

// VT100 event structure
typedef struct
{
    vt100_event_type_t type;
    union {
        char printable;
        char control;
        char escape;
        vt100_csi_t csi;
        vt100_osc_t osc;
        vt100_dcs_t dcs;
        vt100_pm_t pm;
        vt100_apc_t apc;
    } data;
} vt100_event_t;

// Forward declaration for vt100_parser_t
typedef struct vt100_parser vt100_parser_t;

// Callback signature for events
typedef void (*vt100_event_cb)(vt100_parser_t* parser, const vt100_event_t* event, void* user);

// Parser state (no dynamic allocation required)
struct vt100_parser {
    int state;
    void* user;
    vt100_event_cb cb;
    vt100_csi_t csi;
    vt100_osc_t osc;
    vt100_dcs_t dcs;
    vt100_pm_t pm;
    vt100_apc_t apc;
    char esc_intermediate;
};

// Parser API (no dynamic allocation)
void vt100_parser_init(vt100_parser_t* parser, vt100_event_cb cb, void* user);
void vt100_parser_reset(vt100_parser_t* parser);
void vt100_parser_feed(vt100_parser_t* parser, const char* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // VT100_H
