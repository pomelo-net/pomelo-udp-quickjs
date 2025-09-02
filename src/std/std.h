#ifndef POMELO_QJS_STD_SRC_H
#define POMELO_QJS_STD_SRC_H
#include "quickjs.h"
#include "platform/platform.h"
#include "utils/list.h"
#include "pomelo-qjs/core.h"
#ifdef __cplusplus
extern "C" {
#endif


/* Components */
#define POMELO_QJS_STD_TIMER   (1 << 0)  // setTimeout and setInterval
#define POMELO_QJS_STD_CONSOLE (1 << 1)  // console
#define POMELO_QJS_STD_ALL     (POMELO_QJS_STD_TIMER | POMELO_QJS_STD_CONSOLE)


/// @brief Timer object for setTimeout and setInterval
typedef struct pomelo_qjs_timer_s pomelo_qjs_timer_t;


struct pomelo_qjs_std_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Context
    pomelo_qjs_context_t * context;

    /// @brief Flags
    uint32_t flags;

    /// @brief JSClassID of timer
    JSClassID class_timer_id;

    /// @brief Active timers
    pomelo_list_t * active_timers;

    /// @brief Timer pool
    pomelo_pool_t * timer_pool;
};


/// @brief Initialize global std functions
int pomelo_qjs_std_init(
    pomelo_qjs_context_t * context,
    uint32_t components
);


/// @brief Cleanup global std functions
void pomelo_qjs_std_cleanup(pomelo_qjs_context_t * context);

/// @brief Acquire timer
pomelo_qjs_timer_t * pomelo_qjs_std_acquire_timer(pomelo_qjs_context_t * context);


/// @brief Release timer
void pomelo_qjs_std_release_timer(
    pomelo_qjs_context_t * context,
    pomelo_qjs_timer_t * timer
);


#ifdef __cplusplus
}
#endif

#endif // POMELO_QJS_STD_SRC_H
