#include <assert.h>
#include <string.h>
#include "base/allocator.h"
#include "logger/logger.h"
#include "std/std.h"
#include "runtime.h"


#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define POMELO_QJS_RUNTIME_FLAG_LOGGER_INITIALIZED  (1 << 0)
#define POMELO_QJS_RUNTIME_FLAG_STD_INITIALIZED     (1 << 1)


// Custom error dumping function
static void dump_error(JSContext * ctx) {
    assert(ctx != NULL);
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);

    JSValue exception = JS_GetException(ctx);
    if (JS_IsException(exception)) {
        const char * str = JS_ToCString(ctx, exception);
        if (str) {
            pomelo_qjs_logger_log(
                context,
                POMELO_QJS_LOGGER_LEVEL_ERROR,
                str
            );
            JS_FreeCString(ctx, str);
        } else {
            pomelo_qjs_logger_log(
                context,
                POMELO_QJS_LOGGER_LEVEL_ERROR,
                "[Exception]"
            );
        }
        JS_FreeValue(ctx, exception);
    }
}


/// @brief Set import meta
static void set_import_meta_url(
    JSContext * ctx,
    JSValue module_val,
    const char * url
) {
    assert(ctx != NULL);
    assert(url != NULL);

    JSModuleDef * m = JS_VALUE_GET_PTR(module_val);
    if (!m) return;
    
    // Get the module object
    JSValue meta = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta)) {
        JS_FreeValue(ctx, meta);
        return;
    }

    JS_SetPropertyStr(ctx, meta, "url", JS_NewString(ctx, url));
    JS_FreeValue(ctx, meta);
}



static JSValue compile_module(
    JSContext * ctx,
    const char * module_name,
    const char * source,
    size_t source_len
) {
    assert(ctx != NULL);
    assert(module_name != NULL);
    assert(source != NULL);
    
    return JS_Eval(
        ctx,
        source,
        source_len,
        module_name,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );
}


/// @brief Load module value by its name
static JSValue load_module(
    pomelo_qjs_runtime_t * runtime,
    const char * module_name
) {
    // Load the module file
    pomelo_qjs_source_t source;
    int ret = pomelo_qjs_runtime_load_source(runtime, &source, module_name);

    JSContext * ctx = runtime->ctx;
    if (ret < 0) {
        return JS_ThrowPlainError(ctx, "Failed to load: %s", module_name);
    }
    
    // Compile the module
    JSValue module_val = compile_module(
        ctx, module_name, source.content, source.length
    );
    
    // Unload the module file
    pomelo_qjs_runtime_unload_source(runtime, &source);
    
    if (JS_IsException(module_val)) {
        return module_val;
    }

    // Update the import meta
    set_import_meta_url(ctx, module_val, module_name);
    return module_val;
}


// Custom module loader
static JSModuleDef * module_loader(
    JSContext * ctx,
    const char * module_name,
    void * opaque
) {
    (void) ctx;
    (void) opaque;
    JSValue module_val = load_module(
        (pomelo_qjs_runtime_t *) opaque,
        module_name
    );
    if (JS_IsException(module_val)) {
        dump_error(ctx);
        return NULL;
    }

    // Return the module pointer
    return JS_VALUE_GET_PTR(module_val);
}


static JSValue js_module_catch(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    (void) this_val;
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    
    if (argc < 1) {
        pomelo_qjs_logger_log(
            context,
            POMELO_QJS_LOGGER_LEVEL_ERROR,
            "Promise rejected with no reason"
        );
        return JS_UNDEFINED;
    }
    
    // Get the error object as a string
    const char * err_str = JS_ToCString(ctx, argv[0]);
    const char * stack_str = NULL;
    if (err_str) {
        // If available, get the stack trace
        JSValue stack = JS_GetPropertyStr(ctx, argv[0], "stack");
        if (!JS_IsUndefined(stack) && !JS_IsException(stack)) {
            stack_str = JS_ToCString(ctx, stack);
        }

        if (stack_str) {
            pomelo_qjs_logger_log(
                context,
                POMELO_QJS_LOGGER_LEVEL_ERROR,
                "%s\n%s",
                err_str,
                stack_str
            );
            JS_FreeCString(ctx, stack_str);
        } else {
            pomelo_qjs_logger_log(
                context,
                POMELO_QJS_LOGGER_LEVEL_ERROR,
                err_str
            );
        }

        JS_FreeCString(ctx, err_str);
    } else {
        pomelo_qjs_logger_log(
            context,
            POMELO_QJS_LOGGER_LEVEL_ERROR,
            "Promise rejected with an error"
        );
    }

    return JS_UNDEFINED;
}


/// @brief Evaluate a module
static int eval_module(pomelo_qjs_runtime_t * runtime, JSValue module_val) {
    assert(runtime != NULL);
    JSContext * ctx = runtime->ctx;

    // Evaluate the module
    JSValue ret = JS_EvalFunction(ctx, module_val);
    if (JS_IsException(ret)) {
        dump_error(ctx);
        JS_FreeValue(ctx, ret);
        return -1;
    }

    if (JS_IsPromise(ret)) {
        JSValue catch_func = JS_GetPropertyStr(ctx, ret, "catch");
        if (JS_IsException(catch_func)) {
            dump_error(ctx);
            JS_FreeValue(ctx, ret);
            return -1;
        }
        
        JSValue args[] = { runtime->fn_module_catch };
        JSValue res = JS_Call(ctx, catch_func, ret, countof(args), args);
        if (JS_IsException(res)) {
            dump_error(ctx);
        }

        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, catch_func);
    }

    JS_FreeValue(ctx, ret);
    return 0;
}


static void * runtime_calloc(void * opaque, size_t count, size_t size) {
    assert(opaque != NULL);
    pomelo_qjs_runtime_t * runtime = opaque;
    void * ptr = pomelo_allocator_malloc(runtime->allocator, count * size);
    if (!ptr) return NULL;

    memset(ptr, 0, count * size);
    return ptr;
}


static void * runtime_malloc(void * opaque, size_t size) {
    assert(opaque != NULL);
    pomelo_qjs_runtime_t * runtime = opaque;
    return pomelo_allocator_malloc(runtime->allocator, size);
}


static void * runtime_realloc(void * opaque, void * ptr, size_t size) {
    assert(opaque != NULL);
    pomelo_qjs_runtime_t * runtime = opaque;
    return pomelo_allocator_realloc(runtime->allocator, ptr, size);
}


static void runtime_free(void * opaque, void * ptr) {
    assert(opaque != NULL);
    pomelo_qjs_runtime_t * runtime = opaque;
    pomelo_allocator_free(runtime->allocator, ptr);
}


size_t pomelo_qjs_runtime_usable_size(const void * ptr) {
    return pomelo_allocator_sizeof((void *) ptr);
}


static JSMallocFunctions mem_functions = {
    .js_calloc = runtime_calloc,
    .js_malloc = runtime_malloc,
    .js_free = runtime_free,
    .js_realloc = runtime_realloc,
    .js_malloc_usable_size = pomelo_qjs_runtime_usable_size
};


int pomelo_qjs_runtime_init(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_runtime_options_t * options
) {
    assert(runtime != NULL);
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }
    if (!options->platform) {
        return -1; // No platform
    }

    // Create new runtime object
    memset(runtime, 0, sizeof(pomelo_qjs_runtime_t));
    runtime->allocator = allocator;
    runtime->fn_module_catch = JS_UNDEFINED;

    // Create new JS runtime and context
    JSRuntime * rt = JS_NewRuntime2(&mem_functions, runtime);
    if (!rt) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->rt = rt;

    JSContext * ctx = JS_NewContext(rt);
    if (!ctx) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->ctx = ctx;
    // Opaque data of JSContext and JSRuntime will be set to context

    // Create new QJS context
    pomelo_qjs_context_options_t context_options = {
        .allocator = allocator,
        .ctx = ctx,
        .rt = rt,
        .platform = options->platform
    };
    pomelo_qjs_context_t * context =
        pomelo_qjs_context_create(&context_options);
    if (!context) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->context = context;
    pomelo_qjs_context_set_extra(context, runtime);

    // Initialize the logger
    if (pomelo_qjs_logger_init(context) < 0) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->flags |= POMELO_QJS_RUNTIME_FLAG_LOGGER_INITIALIZED;

    // Create catch function
    JSValue fn_catch = JS_NewCFunction2(
        ctx, js_module_catch, "catch", 1, JS_CFUNC_generic, 0
    );
    if (JS_IsException(fn_catch)) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->fn_module_catch = fn_catch;

    // Initialize std
    if (pomelo_qjs_std_init(context, POMELO_QJS_STD_ALL) < 0) {
        pomelo_qjs_runtime_cleanup(runtime);
        return -1;
    }
    runtime->flags |= POMELO_QJS_RUNTIME_FLAG_STD_INITIALIZED;

    // Set module loader with our state
    JS_SetModuleLoaderFunc(rt, NULL, module_loader, runtime);

    // Initialize pomelo module
    js_init_module_pomelo(ctx, "pomelo");
    return 0;
}


void pomelo_qjs_runtime_cleanup(pomelo_qjs_runtime_t * runtime) {
    assert(runtime != NULL);

    if (runtime->flags & POMELO_QJS_RUNTIME_FLAG_LOGGER_INITIALIZED) {
        // Cleanup the logger
        runtime->flags &= ~POMELO_QJS_RUNTIME_FLAG_LOGGER_INITIALIZED;
        pomelo_qjs_logger_cleanup(runtime->context);
    }

    if (runtime->flags & POMELO_QJS_RUNTIME_FLAG_STD_INITIALIZED) {
        // Cleanup std
        runtime->flags &= ~POMELO_QJS_RUNTIME_FLAG_STD_INITIALIZED;
        pomelo_qjs_std_cleanup(runtime->context);
    }

    // Free catch function
    JS_FreeValue(runtime->ctx, runtime->fn_module_catch);

    if (runtime->context) {
        pomelo_qjs_context_destroy(runtime->context);
        runtime->context = NULL;
    }

    if (runtime->ctx) {
        JS_FreeContext(runtime->ctx);
        runtime->ctx = NULL;
    }

    if (runtime->rt) {
        JS_FreeRuntime(runtime->rt);
        runtime->rt = NULL;
    }

    // Do not free runtime here
}


pomelo_qjs_runtime_t * pomelo_qjs_runtime_from_js_context(JSContext * ctx) {
    assert(ctx != NULL);
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    if (!context) return NULL;
    return pomelo_qjs_context_get_extra(context);
}


pomelo_qjs_runtime_t * pomelo_qjs_runtime_from_js_runtime(JSRuntime * rt) {
    assert(rt != NULL);
    pomelo_qjs_context_t * context = JS_GetRuntimeOpaque(rt);
    if (!context) return NULL;
    return pomelo_qjs_context_get_extra(context);
}


void pomelo_qjs_runtime_set_extra(
    pomelo_qjs_runtime_t * runtime,
    void * extra
) {
    assert(runtime != NULL);
    runtime->extra = extra;
}


void * pomelo_qjs_runtime_get_extra(pomelo_qjs_runtime_t * runtime) {
    assert(runtime != NULL);
    return runtime->extra;
}


JSContext * pomelo_qjs_runtime_get_js_context(
    pomelo_qjs_runtime_t * runtime
) {
    assert(runtime != NULL);
    return runtime->ctx;
}


JSRuntime * pomelo_qjs_runtime_get_runtime(
    pomelo_qjs_runtime_t * runtime
) {
    assert(runtime != NULL);
    return runtime->rt;
}


JSValue pomelo_qjs_runtime_load_module(
    pomelo_qjs_runtime_t * runtime,
    const char * module_name
) {
    assert(runtime != NULL);
    assert(module_name != NULL);
    return load_module(runtime, module_name);
}


JSValue pomelo_qjs_runtime_compile_module(
    pomelo_qjs_runtime_t * runtime,
    const char * module_name,
    const char * source
) {
    assert(runtime != NULL);
    assert(module_name != NULL);
    assert(source != NULL);
    
    JSContext * ctx = runtime->ctx;
    size_t source_len = strlen(source);
    return compile_module(ctx, module_name, source, source_len);
}


int pomelo_qjs_runtime_evaluate_module(
    pomelo_qjs_runtime_t * runtime,
    JSValue module_val
) {
    // Evaluate the main module
    assert(runtime != NULL);
    int ret = eval_module(runtime, module_val);
    
    // Execute the main loop
    pomelo_qjs_runtime_main_loop(runtime);

    return ret;
}
