#ifndef POMELO_QUICKJS_SOCKET_SRC_H
#define POMELO_QUICKJS_SOCKET_SRC_H
#include "quickjs.h"
#include "pomelo/api.h"
#include "core.h"
#include "utils/array.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_qjs_socket_s {
    /// @brief The context
    pomelo_qjs_context_t * context;

    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The this of socket. Only available if the socket is working.
    JSValue thiz;

    /// @brief The listener
    JSValue listener;

    /// @brief On connected callback
    JSValue on_connected;

    /// @brief On disconnected callback
    JSValue on_disconnected;

    /// @brief On received callback
    JSValue on_received;

    /// @brief Connect callback functions
    JSValue connect_callback_funcs[2];

    /// @brief this value entry of this socket in context
    pomelo_list_entry_t * thiz_entry;
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the socket module
int pomelo_qjs_init_socket_module(JSContext * ctx, JSModuleDef * m);


/// @brief Initialize the socket
int pomelo_qjs_socket_init(
    pomelo_qjs_socket_t * qjs_socket,
    pomelo_qjs_context_t * context
);


/// @brief Cleanup the socket
void pomelo_qjs_socket_cleanup(pomelo_qjs_socket_t * qjs_socket);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Socket.constructor(listener: SocketListener): Socket
JSValue pomelo_qjs_socket_constructor(
    JSContext * ctx, JSValue new_target, int argc, JSValue * argv
);


/// @brief Finalizer of socket
void pomelo_qjs_socket_finalizer(JSRuntime * rt, JSValue val);


/// @brief Socket.setListener()
JSValue pomelo_qjs_socket_set_listener(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Socket.listen()
JSValue pomelo_qjs_socket_listen(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Socket.connect()
JSValue pomelo_qjs_socket_connect(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Socket.stop()
JSValue pomelo_qjs_socket_stop(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Socket.send()
JSValue pomelo_qjs_socket_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Socket.time()
JSValue pomelo_qjs_socket_time(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Stop the socket
void pomelo_qjs_socket_stop_impl(pomelo_qjs_socket_t * qjs_socket);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_SOCKET_SRC_H
