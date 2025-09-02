#ifndef POMELO_QJS_RUNTIME_UV_SRC_H
#define POMELO_QJS_RUNTIME_UV_SRC_H
#include "pomelo/platforms/platform-uv.h"
#include "pomelo-qjs/runtimes/runtime-uv.h"
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Runtime with uv platform
typedef struct pomelo_qjs_runtime_uv_s pomelo_qjs_runtime_uv_t;


struct pomelo_qjs_runtime_uv_s {
    /// @brief Base runtime
    pomelo_qjs_runtime_t base;

    /// @brief UV loop
    uv_loop_t uv_loop;

    /// @brief Platform
    pomelo_platform_t * platform;
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_RUNTIME_UV_SRC_H
