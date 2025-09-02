#ifndef POMELO_QUICKJS_CHANNEL_SRC_H
#define POMELO_QUICKJS_CHANNEL_SRC_H
#include "quickjs.h"
#include "pomelo/api.h"
#include "core.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_qjs_channel_s {
    /// @brief The context
    pomelo_qjs_context_t * context;

    /// @brief The channel
    pomelo_channel_t * channel;

    /// @brief The this of channel
    JSValue thiz;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the channel module
int pomelo_qjs_init_channel_module(JSContext * ctx, JSModuleDef * m);


/// @brief Create new JS channel. Return null on failure
JSValue pomelo_qjs_channel_new(
    pomelo_qjs_context_t * context,
    pomelo_channel_t * channel
);


/// @brief Initialize the channel
int pomelo_qjs_channel_init(
    pomelo_qjs_channel_t * qjs_channel,
    pomelo_qjs_context_t * context
);


/// @brief Detach the native channel and release the channel to pool
void pomelo_qjs_channel_cleanup(pomelo_qjs_channel_t * qjs_channel);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Finalizer of channel
void pomelo_qjs_channel_finalizer(JSRuntime * rt, JSValue val);


/// @brief Message.mode setter
JSValue pomelo_qjs_channel_set_mode(
    JSContext * ctx, JSValue thiz, JSValue value
);


/// @brief Message.mode getter
JSValue pomelo_qjs_channel_get_mode(JSContext * ctx, JSValue thiz);


/// @brief Message.send()
JSValue pomelo_qjs_channel_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_CHANNEL_SRC_H
