
/**
 * @file vt100.h
 * @brief Incremental VT100/ANSI escape sequence parser interface.
 *
 * This header defines the API for an incremental VT100/ANSI parser.
 * The parser processes input byte-by-byte and emits events for printable
 * characters, control characters, and escape/control sequences.
 * No dynamic memory allocation is required.
 */
#ifndef VT100_H
#define VT100_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum vt100_event_type_t
 * @brief Types of events emitted by the VT100 parser.
 */
typedef enum {
    VT100_EVENT_NONE = 0, /**< No event */
    VT100_EVENT_PRINTABLE, /**< Printable character (ASCII, tab, CR, LF) */
    VT100_EVENT_CONTROL, /**< Control character (C0/C1 except ESC) */
    VT100_EVENT_ESCAPE, /**< ESC + final byte (ESC X) */
    VT100_EVENT_CSI, /**< Control Sequence Introducer (CSI) */
    VT100_EVENT_OSC, /**< Operating System Command (OSC) */
    VT100_EVENT_DCS, /**< Device Control String (DCS) */
    VT100_EVENT_PM, /**< Privacy Message (ESC ^ ... ST) */
    VT100_EVENT_APC, /**< Application Program Command (ESC _ ... ST) */
    VT100_EVENT_IGNORE /**< Unrecognized or ignored input */
} vt100_event_type_t;

/** Maximum number of CSI parameters supported */
#define VT100_MAX_CSI_PARAMS 16
/** Maximum number of CSI intermediate bytes supported */
#define VT100_MAX_CSI_INTERMEDIATES 4
/** Maximum length of OSC string (including null terminator) */
#define VT100_MAX_OSC_STRING 128
/** Maximum length of DCS string (including null terminator) */
#define VT100_MAX_DCS_STRING 128
/** Maximum length of PM string (including null terminator) */
#define VT100_MAX_PM_STRING 128
/** Maximum length of APC string (including null terminator) */
#define VT100_MAX_APC_STRING 128

/**
 * @struct vt100_csi_t
 * @brief Data for a CSI (Control Sequence Introducer) event.
 */
typedef struct {
    char command; /**< Final byte (0x40-0x7E) */
    int params[VT100_MAX_CSI_PARAMS]; /**< Parameters (default 0) */
    int num_params; /**< Number of parameters */
    char intermediates[VT100_MAX_CSI_INTERMEDIATES]; /**< Intermediate bytes */
    int num_intermediates; /**< Number of intermediate bytes */
    char private_leader; /**< Private mode leader (e.g. '?', '>') */
} vt100_csi_t;

/**
 * @struct vt100_pm_t
 * @brief Data for a PM (Privacy Message) event.
 */
typedef struct {
    char string[VT100_MAX_PM_STRING]; /**< Message string */
    size_t length; /**< Length of string */
} vt100_pm_t;

/**
 * @struct vt100_apc_t
 * @brief Data for an APC (Application Program Command) event.
 */
typedef struct {
    char string[VT100_MAX_APC_STRING]; /**< Command string */
    size_t length; /**< Length of string */
} vt100_apc_t;

/**
 * @struct vt100_osc_t
 * @brief Data for an OSC (Operating System Command) event.
 */
typedef struct {
    char string[VT100_MAX_OSC_STRING]; /**< OSC string */
    size_t length; /**< Length of string */
} vt100_osc_t;

/**
 * @struct vt100_dcs_t
 * @brief Data for a DCS (Device Control String) event.
 */
typedef struct {
    char string[VT100_MAX_DCS_STRING]; /**< DCS string */
    size_t length; /**< Length of string */
} vt100_dcs_t;

/**
 * @struct vt100_event_t
 * @brief Event structure emitted by the parser.
 */
typedef struct {
    vt100_event_type_t type; /**< Event type */
    union {
        char printable; /**< Printable character */
        char control; /**< Control character */
        char escape; /**< ESC final byte */
        vt100_csi_t csi; /**< CSI event data */
        vt100_osc_t osc; /**< OSC event data */
        vt100_dcs_t dcs; /**< DCS event data */
        vt100_pm_t pm; /**< PM event data */
        vt100_apc_t apc; /**< APC event data */
    } data;
} vt100_event_t;

/**
 * @brief Opaque parser state structure.
 */
typedef struct vt100_parser vt100_parser_t;

/**
 * @brief Callback signature for parser events.
 *
 * @param parser Pointer to parser instance
 * @param event  Pointer to event data
 * @param user   User data pointer (from parser init)
 */
typedef void (*vt100_event_cb)(vt100_parser_t* parser, const vt100_event_t* event, void* user);

/**
 * @struct vt100_parser
 * @brief Parser state structure (opaque to users).
 *
 * Do not access fields directly; use API functions.
 */
struct vt100_parser {
    int state; /**< Internal state */
    void* user; /**< User data pointer */
    vt100_event_cb cb; /**< Event callback */
    vt100_csi_t csi; /**< CSI state */
    vt100_osc_t osc; /**< OSC state */
    vt100_dcs_t dcs; /**< DCS state */
    vt100_pm_t pm; /**< PM state */
    vt100_apc_t apc; /**< APC state */
    char esc_intermediate; /**< ESC intermediate byte */
};

/**
 * @brief Initialize a VT100 parser instance.
 *
 * @param parser Pointer to parser state (must not be NULL)
 * @param cb     Event callback function (may be NULL)
 * @param user   User data pointer (passed to callback)
 */
void vt100_parser_init(vt100_parser_t* parser, vt100_event_cb cb, void* user);

/**
 * @brief Reset parser state to initial (ground) state.
 *
 * @param parser Pointer to parser state
 */
void vt100_parser_reset(vt100_parser_t* parser);

/**
 * @brief Feed input data to the parser.
 *
 * @param parser Pointer to parser state
 * @param data   Input data buffer
 * @param len    Number of bytes to process
 */
void vt100_parser_feed(vt100_parser_t* parser, const char* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // VT100_H
