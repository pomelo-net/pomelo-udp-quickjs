#ifndef POMELO_QJS_CORE_H
#define POMELO_QJS_CORE_H
#include "quickjs.h"
#include "pomelo/allocator.h"
#include "pomelo/platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The context of module
typedef struct pomelo_qjs_context_s pomelo_qjs_context_t;

/// @brief The options of pomelo quickjs context
typedef struct pomelo_qjs_context_options_s pomelo_qjs_context_options_t;


struct pomelo_qjs_context_options_s {
    /// @brief The allocator of module
    pomelo_allocator_t * allocator;

    /// @brief The runtime of quickjs
    JSRuntime * rt;

    /// @brief The context of quickjs
    JSContext * ctx;

    /// @brief The platform
    pomelo_platform_t * platform;

    /* Optional */

    /// @brief The message capacity
    size_t message_capacity;

    /// @brief The pool of sockets
    size_t pool_socket_max;

    /// @brief The pool of messages
    size_t pool_message_max;

    /// @brief The pool of sessions
    size_t pool_session_max;

    /// @brief The pool of channels
    size_t pool_channel_max;
};


/// @brief Initialize pomelo module
JSModuleDef * js_init_module_pomelo(JSContext * ctx, const char * name);


/// @brief Create the context of pomelo quickjs
pomelo_qjs_context_t * pomelo_qjs_context_create(
    pomelo_qjs_context_options_t * options
);


/// @brief Destroy the context of pomelo quickjs
void pomelo_qjs_context_destroy(pomelo_qjs_context_t * context);


/// @brief Get the js context
JSContext * pomelo_qjs_context_get_js_context(pomelo_qjs_context_t * context);


/// @brief Get the js runtime
JSRuntime * pomelo_qjs_context_get_js_runtime(pomelo_qjs_context_t * context);


/// @brief Set context extra
void pomelo_qjs_context_set_extra(pomelo_qjs_context_t * context, void * extra);


/// @brief Get context extra
void * pomelo_qjs_context_get_extra(pomelo_qjs_context_t * context);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_CORE_H
