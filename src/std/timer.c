#include <assert.h>
#include "timer.h"
#include "core/context.h"
#include "std.h"


int pomelo_qjs_timer_init(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    JSContext * ctx = context->ctx;
    JSRuntime * rt = context->rt;

    pomelo_qjs_std_t * std = context->std;
    assert(std != NULL);

    JSValue global = JS_GetGlobalObject(ctx);
    int ret = pomelo_qjs_timer_init_global_functions(ctx, global);
    JS_FreeValue(ctx, global);
    if (ret < 0) {
        return ret; // Failed to initialize global functions
    }

    // Initialize timer class
    JSClassID class_id = 0;
    if (JS_NewClassID(context->rt, &class_id) < 0) {
        return -1; // Failed to create new class ID
    }
    std->class_timer_id = class_id;

    JSClassDef class_def = {
        .class_name = "Timer",
        .finalizer = pomelo_qjs_timer_finalizer,
    };
    if (JS_NewClass(rt, class_id, &class_def) < 0) {
        return -1;
    }

    // Create prototype for class
    // JSValue proto = JS_NewObject(ctx);
    // if (JS_IsException(proto)) {
    //     return -1; // Failed to create prototype
    // }

    // // No methods for this class
    // JSValue class_timer = JS_NewCFunction2(
    //     ctx, pomelo_qjs_timer_constructor, "Timer", 0, JS_CFUNC_constructor, 0
    // );
    // if (JS_IsException(class_timer)) return -1; // Failed to create constructor

    // JS_SetConstructor(ctx, class_timer, proto);

    // // Set the class prototype
    // JS_SetClassProto(ctx, class_id, proto);
    return 0;
}


void pomelo_qjs_timer_cleanup(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    pomelo_qjs_std_t * std = context->std;
    assert(std != NULL);

    pomelo_qjs_timer_t * timer = NULL;
    while (pomelo_list_pop_front(std->active_timers, &timer) == 0) {
        timer->active_entry = NULL; // Detach the entry first
        pomelo_qjs_timer_stop(timer);
    }
}


int pomelo_qjs_timer_init_global_functions(JSContext * ctx, JSValue global) {
    // setTimeout
    JSValue fn_set_timeout = JS_NewCFunction(
        ctx, pomelo_qjs_timer_set_timeout, "setTimeout", 2
    );
    if (JS_IsException(fn_set_timeout)) return -1;
    if (JS_SetPropertyStr(ctx, global, "setTimeout", fn_set_timeout) < 0) {
        JS_FreeValue(ctx, fn_set_timeout);
        return -1;
    }

    // setInterval
    JSValue fn_set_interval = JS_NewCFunction(
        ctx, pomelo_qjs_timer_set_interval, "setInterval", 2
    );
    if (JS_IsException(fn_set_interval)) return -1;
    if (JS_SetPropertyStr(ctx, global, "setInterval", fn_set_interval) < 0) {
        JS_FreeValue(ctx, fn_set_interval);
        return -1;
    }

    /* clearTimeout equals clearInterval in native */

    // clearTimeout
    JSValue fn_clear_timeout = JS_NewCFunction(
        ctx, pomelo_qjs_timer_clear, "clearTimeout", 1
    );
    if (JS_IsException(fn_clear_timeout)) return -1;
    if (JS_SetPropertyStr(ctx, global, "clearTimeout", fn_clear_timeout) < 0) {
        JS_FreeValue(ctx, fn_clear_timeout);
        return -1;
    }

    // clearInterval
    JSValue fn_clear_interval = JS_NewCFunction(
        ctx, pomelo_qjs_timer_clear, "clearInterval", 1
    );
    if (JS_IsException(fn_clear_interval)) return -1;
    if (JS_SetPropertyStr(ctx, global, "clearInterval", fn_clear_interval) < 0) {
        JS_FreeValue(ctx, fn_clear_interval);
        return -1;
    }

    return 0;
}


JSValue pomelo_qjs_timer_set_timeout(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    return pomelo_qjs_timer_set(
        ctx, this_val, argc, argv, POMELO_QJS_TIMER_TYPE_TIMEOUT
    );
}


JSValue pomelo_qjs_timer_set_interval(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    return pomelo_qjs_timer_set(
        ctx, this_val, argc, argv, POMELO_QJS_TIMER_TYPE_INTERVAL
    );
}


JSValue pomelo_qjs_timer_set(
    JSContext * ctx,
    JSValue thiz, // context, not the timer
    int argc,
    JSValue * argv,
    pomelo_qjs_timer_type type
) {
    (void) thiz;
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    pomelo_qjs_std_t * std = context->std;
    assert(std != NULL);

    if (argc < 1) return JS_UNDEFINED; // Nothing to do
    JSValue callback = argv[0];
    if (!JS_IsFunction(ctx, callback)) {
        return JS_ThrowTypeError(ctx, "Callback must be a function");
    }

    uint64_t time_ms = 0;
    if (argc > 1) {
        // Parse timeout
        if (JS_IsNumber(argv[1]) || JS_IsString(argv[1])) {
            int64_t value = 0;
            int ret = JS_ToInt64(ctx, &value, argv[1]);
            if (ret < 0) {
                time_ms = 0;
            } else {
                time_ms = (value > 0) ? (uint64_t) value : 0;
            }
        } else if (JS_IsBigInt(argv[1])) {
            int64_t value = 0;
            int ret = JS_ToBigInt64(ctx, &value, argv[1]);
            if (ret < 0) {
                time_ms = 0;
            } else {
                time_ms = (value > 0) ? (uint64_t) value : 0;
            }
        }
    }
    
    // Now create new timer
    JSValue self = JS_NewObjectClass(ctx, context->std->class_timer_id);
    if (JS_IsException(self)) return self;

    pomelo_qjs_timer_t * timer = pomelo_qjs_std_acquire_timer(context);
    if (!timer) {
        JS_FreeValue(ctx, self);
        return JS_ThrowTypeError(ctx, "Failed to acquire timer");
    }
    JS_SetOpaque(self, timer);

    timer->context = context;
    timer->self = self; // Weak reference
    timer->callback = JS_DupValue(ctx, callback);
    timer->type = type;

    int ret = pomelo_platform_timer_start(
        context->platform,
        (pomelo_platform_timer_entry) pomelo_qjs_timer_entry, // entry
        time_ms, // timeout
        time_ms, // repeat
        timer, // user_data
        &timer->timer_handle
    );
    if (ret < 0) {
        JS_FreeValue(ctx, self);
        pomelo_qjs_std_release_timer(context, timer);
        return JS_ThrowTypeError(ctx, "Failed to start timer");
    }

    timer->active_entry = pomelo_list_push_back(std->active_timers, timer);
    return self;
}


JSValue pomelo_qjs_timer_clear(
    JSContext * ctx,
    JSValue thiz, // context, not the timer
    int argc,
    JSValue * argv
) {
    (void) thiz;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    if (argc < 1) return JS_UNDEFINED;

    JSValue self = argv[0];
    if (!JS_IsObject(self)) return JS_UNDEFINED;

    // Get the timer
    pomelo_qjs_timer_t * timer =
        JS_GetOpaque(self, context->std->class_timer_id);
    if (!timer) return JS_UNDEFINED;

    // Stop the timer
    pomelo_qjs_timer_stop(timer);
    return JS_UNDEFINED;
}


JSValue pomelo_qjs_timer_constructor(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    // Empty constructor
    (void) ctx;
    (void) argc;
    (void) argv;
    return thiz;
}


void pomelo_qjs_timer_finalizer(JSRuntime * rt, JSValue val) {
    pomelo_qjs_context_t * context = JS_GetRuntimeOpaque(rt);
    assert(context != NULL);
    pomelo_qjs_timer_t * timer =
        JS_GetOpaque(val, context->std->class_timer_id);
    if (timer) {
        // Detach JS object from timer
        timer->self = JS_UNDEFINED;
    }
}


void pomelo_qjs_timer_entry(pomelo_qjs_timer_t * timer) {
    assert(timer != NULL);
    JSContext * ctx = timer->context->ctx;
    assert(JS_IsFunction(ctx, timer->callback));
    JSValue callback = JS_DupValue(ctx, timer->callback);

    // Clear the timer (if needed)
    if (timer->type == POMELO_QJS_TIMER_TYPE_TIMEOUT) {
        pomelo_qjs_timer_stop(timer);
    }

    // Call the callback
    JSValue ret = JS_Call(ctx, callback, JS_NULL, 0, NULL);
    JS_FreeValue(ctx, callback);
    JS_FreeValue(ctx, ret);
}


void pomelo_qjs_timer_stop(pomelo_qjs_timer_t * timer) {
    assert(timer != NULL);
    pomelo_qjs_context_t * context = timer->context;
    assert(context != NULL);

    // Stop the timer
    pomelo_platform_timer_stop(context->platform, &timer->timer_handle);

    // Detach the timer from the object
    if (JS_IsObject(timer->self)) {
        JS_SetOpaque(timer->self, NULL);
        timer->self = JS_UNDEFINED;
    }

    // Release the callback
    JS_FreeValue(context->ctx, timer->callback);
    timer->callback = JS_UNDEFINED;

    if (timer->active_entry) {
        pomelo_list_remove(context->std->active_timers, timer->active_entry);
        timer->active_entry = NULL;
    }

    // Release the timer
    pomelo_qjs_std_release_timer(context, timer);
}
