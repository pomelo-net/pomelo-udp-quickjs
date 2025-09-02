#ifndef POMELO_QJS_RUNTIME_UV_H
#define POMELO_QJS_RUNTIME_UV_H
#include "pomelo-qjs/runtime.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Option to create runtime with uv
typedef struct pomelo_qjs_runtime_uv_options_s pomelo_qjs_runtime_uv_options_t;


struct pomelo_qjs_runtime_uv_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;
};


/// @brief Create runtime with uv
/// @return Return the runtime if success or NULL if failed
pomelo_qjs_runtime_t * pomelo_qjs_runtime_uv_create(
    pomelo_qjs_runtime_uv_options_t * options
);


/// @brief Destroy runtime with uv
void pomelo_qjs_runtime_uv_destroy(pomelo_qjs_runtime_t * runtime);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_RUNTIME_UV_H
