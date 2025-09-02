#include <assert.h>
#include <string.h>
#include "core/context.h"
#include "std.h"
#include "timer.h"
#include "console.h"


int pomelo_qjs_std_init(
    pomelo_qjs_context_t * context,
    uint32_t components
) {
    assert(context != NULL);
    if (context->std) return 0; // Initialized, nothing to do

    pomelo_allocator_t * allocator = context->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_qjs_std_t * std =
        pomelo_allocator_malloc_t(allocator, pomelo_qjs_std_t);
    if (!std) {
        return -1; // Failed to allocate new std
    }
    memset(std, 0, sizeof(pomelo_qjs_std_t));
    std->allocator = allocator;
    std->context = context;

    // Attach the STD
    context->std = std;

    // Create list of active timers
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_qjs_timer_t *)
    };
    std->active_timers = pomelo_list_create(&list_options);
    if (!std->active_timers) {
        pomelo_qjs_std_cleanup(context);
        return -1;
    }

    // Initialize timer
    if (components & POMELO_QJS_STD_TIMER) {
        if (pomelo_qjs_timer_init(context) < 0) {
            pomelo_qjs_std_cleanup(context);
            return -1;
        }
        std->flags |= POMELO_QJS_STD_TIMER;
    }

    // Create timer pool
    pomelo_pool_root_options_t pool_options = {
        .allocator = allocator,
        .zero_init = true,
        .element_size = sizeof(pomelo_qjs_timer_t)
    };
    std->timer_pool = pomelo_pool_root_create(&pool_options);
    if (!std->timer_pool) {
        pomelo_qjs_std_cleanup(context);
        return -1;
    }

    // Initialize console
    if (components & POMELO_QJS_STD_CONSOLE) {
        if (pomelo_qjs_console_init(context) < 0) {
            pomelo_qjs_std_cleanup(context);
            return -1;
        }
        std->flags |= POMELO_QJS_STD_CONSOLE;
    }

    return 0;
}


void pomelo_qjs_std_cleanup(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    pomelo_qjs_std_t * std = context->std;
    assert(std != NULL);

    // Cleanup the timer
    if (std->flags & POMELO_QJS_STD_TIMER) {
        std->flags &= ~POMELO_QJS_STD_TIMER;
        pomelo_qjs_timer_cleanup(context);
    }

    // Cleanup the console
    if (std->flags & POMELO_QJS_STD_CONSOLE) {
        std->flags &= ~POMELO_QJS_STD_CONSOLE;
        pomelo_qjs_console_cleanup(context);
    }

    if (std->timer_pool) {
        pomelo_pool_destroy(std->timer_pool);
        std->timer_pool = NULL;
    }

    if (std->active_timers) {
        pomelo_list_destroy(std->active_timers);
        std->active_timers = NULL;
    }

    // Free itself
    pomelo_allocator_free(std->allocator, std);
}


pomelo_qjs_timer_t * pomelo_qjs_std_acquire_timer(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->std->timer_pool, NULL);
}


void pomelo_qjs_std_release_timer(
    pomelo_qjs_context_t * context,
    pomelo_qjs_timer_t * timer
) {
    assert(context != NULL);
    pomelo_pool_release(context->std->timer_pool, timer);
}
