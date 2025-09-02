#ifndef POMELO_QJS_TIMER_SRC_H
#define POMELO_QJS_TIMER_SRC_H
#include "std.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    POMELO_QJS_TIMER_TYPE_TIMEOUT,  // Timer created by setTimeout
    POMELO_QJS_TIMER_TYPE_INTERVAL, // Timer created by setInterval
} pomelo_qjs_timer_type;


struct pomelo_qjs_timer_s {
    /// @brief Context
    pomelo_qjs_context_t * context;

    /// @brief Handle of timer
    pomelo_platform_handle_t timer_handle;

    /// @brief The JS object of timer. This is a weak reference.
    JSValue self;

    /// @brief The callback of timer
    JSValue callback;

    /// @brief The type of timer
    pomelo_qjs_timer_type type;

    /// @brief List entry of this timer in active timers
    pomelo_list_entry_t * active_entry;
};


/// @brief Initialize timer
int pomelo_qjs_timer_init(pomelo_qjs_context_t * context);


/// @brief Cleanup timer
void pomelo_qjs_timer_cleanup(pomelo_qjs_context_t * context);


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize global functions
int pomelo_qjs_timer_init_global_functions(JSContext * ctx, JSValue global);

/// @brief setTimeout function
JSValue pomelo_qjs_timer_set_timeout(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);

/// @brief setInterval function
JSValue pomelo_qjs_timer_set_interval(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);

/// @brief Set timer
JSValue pomelo_qjs_timer_set(
    JSContext * ctx,
    JSValue thiz,
    int argc,
    JSValue * argv,
    pomelo_qjs_timer_type type
);

/// @brief Clear timer
JSValue pomelo_qjs_timer_clear(
    JSContext * ctx,
    JSValue thiz,
    int argc,
    JSValue * argv
);

/// @brief Timer constructor
JSValue pomelo_qjs_timer_constructor(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Timer finalizer
void pomelo_qjs_timer_finalizer(JSRuntime * rt, JSValue val);


/// @brief Timer entry
void pomelo_qjs_timer_entry(pomelo_qjs_timer_t * timer);


/// @brief Stop timer
void pomelo_qjs_timer_stop(pomelo_qjs_timer_t * timer);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_TIMER_SRC_H
