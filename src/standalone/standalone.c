#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "quickjs.h"
#include "logger/logger.h"
#include "runtime/runtime.h"
#include "runtime/runtime-uv.h"


int main(int argc, char * argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <index.js>\n", argv[0]);
        return -1;
    }

    const char * filename = argv[1];
    pomelo_allocator_t * allocator = pomelo_allocator_default();
#ifndef NDEBUG
    size_t allocated_bytes = pomelo_allocator_allocated_bytes(allocator);
#endif
    pomelo_qjs_runtime_uv_options_t runtime_options = {
        .allocator = allocator
    };

    pomelo_qjs_runtime_t * runtime =
        pomelo_qjs_runtime_uv_create(&runtime_options);
    if (!runtime) return -1; // Failed to create new runtime

    // Set allocator as the associated data of runtime
    pomelo_qjs_runtime_set_extra(runtime, allocator);
    JSContext * ctx = pomelo_qjs_runtime_get_js_context(runtime);

    JSValue module_val = pomelo_qjs_runtime_load_module(runtime, filename);
    if (JS_IsException(module_val)) {
        JS_FreeValue(ctx, module_val);
        return -1;
    }

    // Run the file
    int ret = pomelo_qjs_runtime_evaluate_module(runtime, module_val);

    // No need to free module_val

    // Destroy the runtime
    pomelo_qjs_runtime_uv_destroy(runtime);
#ifndef NDEBUG
    size_t allocated_bytes_after = pomelo_allocator_allocated_bytes(allocator);
    assert(allocated_bytes_after == allocated_bytes);
#endif
    return ret;
}


int pomelo_qjs_runtime_load_source(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_source_t * source,
    const char * module_name
) {
    assert(runtime != NULL);
    assert(source != NULL);
    assert(module_name != NULL);

    FILE * file = fopen(module_name, "rb");
    if (!file) return -1;
    
    if (fseek(file, 0, SEEK_END) < 0) {
        fclose(file);
        return -1;
    }
    
    long fsize = ftell(file);
    if (fsize < 0) {
        fclose(file);
        return -1;
    }
    
    if (fseek(file, 0, SEEK_SET) < 0) {
        fclose(file);
        return -1;
    }
    
    pomelo_allocator_t * allocator = pomelo_qjs_runtime_get_extra(runtime);
    uint8_t * buf = pomelo_allocator_malloc(allocator, (size_t) (fsize + 1));
    // Additional zero trail
    if (!buf) {
        fclose(file);
        return -1;
    }
    
    size_t read = fread(buf, 1, fsize, file);
    if (ferror(file)) {
        fclose(file);
        pomelo_allocator_free(allocator, buf);
        return -1;
    }
    
    fclose(file);
    if (read != (size_t) fsize) {
        pomelo_allocator_free(allocator, buf);
        return -1;
    }

    buf[fsize] = '\0';
    source->content = (const char *) buf;
    source->length = (size_t) fsize;
    source->handle = buf;
    return 0;
}


void pomelo_qjs_runtime_unload_source(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_source_t * source
) {
    assert(runtime != NULL);
    assert(source != NULL);
    pomelo_allocator_t * allocator = pomelo_qjs_runtime_get_extra(runtime);
    pomelo_allocator_free(allocator, source->handle);
}


/**
 * Default implementation of logger
 */

 int pomelo_qjs_logger_init(pomelo_qjs_context_t * context) {
    (void) context;
    return 0;
}


void pomelo_qjs_logger_cleanup(pomelo_qjs_context_t * context) {
    (void) context;
    // Ingore
}


void pomelo_qjs_logger_log(
    pomelo_qjs_context_t * context,
    pomelo_qjs_logger_level level,
    const char * fmt,
    ...
) {
    (void) context;
    FILE * output = (
        level == POMELO_QJS_LOGGER_LEVEL_ERROR || 
        level == POMELO_QJS_LOGGER_LEVEL_WARN
    ) ? stderr : stdout;

    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
    
    // Add newline and flush
    fputc('\n', output);
    fflush(output);
}
