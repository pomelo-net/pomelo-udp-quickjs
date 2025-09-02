#ifndef POMELO_QJS_RUNTIME_H
#define POMELO_QJS_RUNTIME_H
#include "quickjs.h"
#include "pomelo/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Runtime
typedef struct pomelo_qjs_runtime_s pomelo_qjs_runtime_t;


/// @brief Set runtime extra data
void pomelo_qjs_runtime_set_extra(
    pomelo_qjs_runtime_t * runtime,
    void * extra
);


/// @brief Get extra data
void * pomelo_qjs_runtime_get_extra(pomelo_qjs_runtime_t * runtime);


/// @brief Get QuickJS context
JSContext * pomelo_qjs_runtime_get_js_context(pomelo_qjs_runtime_t * runtime);


/// @brief Get QuickJS runtime
JSRuntime * pomelo_qjs_runtime_get_runtime(pomelo_qjs_runtime_t * runtime);


/// @brief Load a module by name
JSValue pomelo_qjs_runtime_load_module(
    pomelo_qjs_runtime_t * runtime,
    const char * module_name
);


/// @brief Compile a module from source
JSValue pomelo_qjs_runtime_compile_module(
    pomelo_qjs_runtime_t * runtime,
    const char * module_name,
    const char * module_source
);


/// @brief Evaluate a module
int pomelo_qjs_runtime_evaluate_module(
    pomelo_qjs_runtime_t * runtime,
    JSValue module_val
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_RUNTIME_H
