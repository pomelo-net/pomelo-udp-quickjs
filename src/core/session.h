#ifndef POMELO_QUICKJS_SESSION_SRC_H
#define POMELO_QUICKJS_SESSION_SRC_H
#include "quickjs.h"
#include "pomelo/api.h"
#include "core.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_qjs_session_s {
    /// @brief The context
    pomelo_qjs_context_t * context;

    /// @brief The session
    pomelo_session_t * session;

    /// @brief The this of session
    JSValue thiz;

    /// @brief The array of channels
    JSValue channels;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the session module
int pomelo_qjs_init_session_module(JSContext * ctx, JSModuleDef * m);


/// @brief Create new JS session
JSValue pomelo_qjs_session_new(
    pomelo_qjs_context_t * context,
    pomelo_session_t * session
);


/// @brief Initialize the session
int pomelo_qjs_session_init(
    pomelo_qjs_session_t * qjs_session,
    pomelo_qjs_context_t * context
);


/// @brief Detach the native session and release the session to pool
void pomelo_qjs_session_cleanup(pomelo_qjs_session_t * qjs_session);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief send(channelIndex: number, message: Message): boolean
JSValue pomelo_qjs_session_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief readonly Session.id: number
JSValue pomelo_qjs_session_get_id(JSContext * ctx, JSValue thiz);


/// @brief Session.disconnect(): void
JSValue pomelo_qjs_session_disconnect(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Session.setChannelMode(channelIndex: number, mode: ChannelMode)
JSValue pomelo_qjs_session_set_channel_mode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Session.getChannelMode(channelIndex: number): ChannelMode
JSValue pomelo_qjs_session_get_channel_mode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);

/// @brief Session.rtt(): RTT
JSValue pomelo_qjs_session_rtt(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Session.channels: Channel[]
JSValue pomelo_qjs_session_get_channels(JSContext * ctx, JSValue thiz);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_SESSION_SRC_H
