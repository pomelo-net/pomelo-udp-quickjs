#include <assert.h>
#include <string.h>
#include "core/core.h"
#include "runtime-uv.h"


pomelo_qjs_runtime_t * pomelo_qjs_runtime_uv_create(
    pomelo_qjs_runtime_uv_options_t * options
) {
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_qjs_runtime_uv_t * runtime =
        pomelo_allocator_malloc_t(allocator, pomelo_qjs_runtime_uv_t);
    if (!runtime) return NULL; // Failed to allocate new runtime
    memset(runtime, 0, sizeof(pomelo_qjs_runtime_uv_t));

    // Initialize UV loop
    uv_loop_init(&runtime->uv_loop);

    // Create new platform
    pomelo_platform_uv_options_t platform_options = {
        .allocator = allocator,
        .uv_loop = &runtime->uv_loop
    };
    pomelo_platform_t * platform = pomelo_platform_uv_create(&platform_options);
    if (!platform) {
        pomelo_qjs_runtime_uv_destroy((pomelo_qjs_runtime_t *) runtime);
        return NULL;
    }
    runtime->platform = platform;

    // Initialize base runtime
    pomelo_qjs_runtime_options_t runtime_options = {
        .allocator = allocator,
        .platform = platform
    };
    if (pomelo_qjs_runtime_init(&runtime->base, &runtime_options) < 0) {
        pomelo_qjs_runtime_uv_destroy((pomelo_qjs_runtime_t *) runtime);
        return NULL;
    }

    return &runtime->base;
}


void pomelo_qjs_runtime_uv_destroy(pomelo_qjs_runtime_t * runtime) {
    assert(runtime != NULL);
    // Cleanup base first
    pomelo_qjs_runtime_cleanup(runtime);

    pomelo_qjs_runtime_uv_t * impl = (pomelo_qjs_runtime_uv_t *) runtime;
    // Destroy the platform
    if (impl->platform) {
        pomelo_platform_uv_destroy(impl->platform);
        impl->platform = NULL;
    }

    // Free the runtime
    pomelo_allocator_free(runtime->allocator, impl);
}


void pomelo_qjs_runtime_main_loop(pomelo_qjs_runtime_t * runtime) {
    assert(runtime != NULL);

    pomelo_qjs_runtime_uv_t * impl = (pomelo_qjs_runtime_uv_t *) runtime;
    uv_loop_t * uv_loop = pomelo_platform_uv_get_uv_loop(impl->platform);
    assert(uv_loop != NULL);

    JSRuntime * rt = runtime->rt;
    assert(rt != NULL);

    bool running = true;
    while (running) {
        // Execute all pending jobs first
        while (JS_IsJobPending(rt)) {
            JSContext * ctx;
            JS_ExecutePendingJob(rt, &ctx);
        }

        // Then poll IO
        running = (uv_run(uv_loop, UV_RUN_ONCE) > 0);
        running |= JS_IsJobPending(rt);
    }
}


JSValue pomelo_qjs_platform_statistic(
    JSContext * ctx, pomelo_platform_t * platform
) {
    assert(ctx != NULL);
    assert(platform != NULL);

    pomelo_statistic_platform_uv_t uv_statistic = {0};
    pomelo_platform_uv_statistic(platform, &uv_statistic);

    // Add platform stats (partially implemented)
    JSValue js_platform = JS_NewObject(ctx);
    if (JS_IsException(js_platform)) return js_platform;

    // Initialize with default values as they're not in the C struct
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "timers",
        JS_NewUint32(ctx, (uint32_t) uv_statistic.timers)
    );
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "workerTasks",
        JS_NewUint32(ctx, (uint32_t) uv_statistic.worker_tasks)
    );
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "deferredTasks",
        JS_NewUint32(ctx, (uint32_t) uv_statistic.deferred_tasks)
    );
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "sendCommands",
        JS_NewUint32(ctx, (uint32_t) uv_statistic.send_commands)
    );
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "sentBytes",
        JS_NewBigUint64(ctx, uv_statistic.sent_bytes)
    );
    JS_SetPropertyStr(
        ctx,
        js_platform,
        "recvBytes",
        JS_NewBigUint64(ctx, uv_statistic.recv_bytes)
    );

    return js_platform;
}
