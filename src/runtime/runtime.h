#ifndef POMELO_QJS_RUNTIME_SRC_H
#define POMELO_QJS_RUNTIME_SRC_H
#include <stdbool.h>
#include "pomelo-qjs/runtime.h"
#include "pomelo-qjs/core.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Runtime options
typedef struct pomelo_qjs_runtime_options_s pomelo_qjs_runtime_options_t;


/// @brief Source information
typedef struct pomelo_qjs_source_s pomelo_qjs_source_t;


struct pomelo_qjs_source_s {
    /// @brief The source content
    const char * content;
    
    /// @brief The source length
    size_t length;

    /// @brief The handle to unload later
    void * handle;
};


struct pomelo_qjs_runtime_s {
    /// @brief Extra data
    void * extra;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Context
    pomelo_qjs_context_t * context;

    /// @brief QuickJS runtime
    JSRuntime * rt;

    /// @brief QuickJS context
    JSContext * ctx;

    /// @brief Catch function when resolving module
    JSValue fn_module_catch;

    /// @brief Flags which used internally
    uint32_t flags;
};


struct pomelo_qjs_runtime_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Platform
    pomelo_platform_t * platform;
};


/// @brief Initialize runtime
int pomelo_qjs_runtime_init(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_runtime_options_t * options
);


/// @brief Cleanup a runtime
void pomelo_qjs_runtime_cleanup(pomelo_qjs_runtime_t * runtime);


/// @brief Get the runtime from JS context
pomelo_qjs_runtime_t * pomelo_qjs_runtime_from_js_context(JSContext * ctx);


/// @brief Get the runtime from JS runtime
pomelo_qjs_runtime_t * pomelo_qjs_runtime_from_js_runtime(JSRuntime * rt);


/// @brief [External linkage] Load a module content
/// @return Return 0 if success or -1 if failed
int pomelo_qjs_runtime_load_source(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_source_t * source,
    const char * module_name
);


/// @brief [External linkage] Unload a module source
void pomelo_qjs_runtime_unload_source(
    pomelo_qjs_runtime_t * runtime,
    pomelo_qjs_source_t * source
);


/// @brief [External linkage] Main loop of runtime
void pomelo_qjs_runtime_main_loop(pomelo_qjs_runtime_t * runtime);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_RUNTIME_SRC_H
