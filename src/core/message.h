#ifndef POMELO_QUICKJS_MESSAGE_SRC_H
#define POMELO_QUICKJS_MESSAGE_SRC_H
#include "quickjs.h"
#include "pomelo/api.h"
#include "core.h"
#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_qjs_message_s {
    /// @brief The context
    pomelo_qjs_context_t * context;

    /// @brief The message
    pomelo_message_t * message;

    /// @brief The this of message. This is always a weak reference.
    JSValue thiz;
};


struct pomelo_qjs_send_info_s {
    /// @brief The context
    pomelo_qjs_context_t * context;

    /// @brief The sending message
    pomelo_message_t * message;

    /// @brief The JS message
    JSValue js_message;

    /// @brief The promise functions
    JSValue promise_funcs[2];
};


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the message module
int pomelo_qjs_init_message_module(JSContext * ctx, JSModuleDef * m);


/// @brief Create new JS message
JSValue pomelo_qjs_message_new(
    pomelo_qjs_context_t * context,
    pomelo_message_t * message
);


/// @brief Initialize the message
int pomelo_qjs_message_init(
    pomelo_qjs_message_t * qjs_message,
    pomelo_qjs_context_t * context
);


/// @brief Detach the native message and release the message to pool
void pomelo_qjs_message_cleanup(pomelo_qjs_message_t * qjs_message);


/// @brief Initialize the send info. Return new promise for the send info
JSValue pomelo_qjs_send_info_init(
    pomelo_qjs_send_info_t * send_info,
    pomelo_qjs_context_t * context,
    pomelo_qjs_message_t * qjs_message
);


/// @brief Finalize the send info
void pomelo_qjs_send_info_finalize(pomelo_qjs_send_info_t * send_info);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Message.constructor()
JSValue pomelo_qjs_message_constructor(
    JSContext * ctx, JSValue new_target, int argc, JSValue * argv
);


/// @brief Finalizer of message
void pomelo_qjs_message_finalizer(JSRuntime * rt, JSValue val);


/// @brief Message.reset()
JSValue pomelo_qjs_message_reset(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.size()
JSValue pomelo_qjs_message_size(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.read()
JSValue pomelo_qjs_message_read(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readUint8()
JSValue pomelo_qjs_message_read_uint8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readUint16()
JSValue pomelo_qjs_message_read_uint16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readUint32()
JSValue pomelo_qjs_message_read_uint32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readUint64()
JSValue pomelo_qjs_message_read_uint64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readInt8()
JSValue pomelo_qjs_message_read_int8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readInt16()
JSValue pomelo_qjs_message_read_int16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readInt32()
JSValue pomelo_qjs_message_read_int32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readInt64()
JSValue pomelo_qjs_message_read_int64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readFloat32()
JSValue pomelo_qjs_message_read_float32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.readFloat64()
JSValue pomelo_qjs_message_read_float64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.write()
JSValue pomelo_qjs_message_write(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeUint8()
JSValue pomelo_qjs_message_write_uint8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeUint16()
JSValue pomelo_qjs_message_write_uint16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeUint32()
JSValue pomelo_qjs_message_write_uint32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeUint64()
JSValue pomelo_qjs_message_write_uint64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeInt8()
JSValue pomelo_qjs_message_write_int8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeInt16()
JSValue pomelo_qjs_message_write_int16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeInt32()
JSValue pomelo_qjs_message_write_int32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeInt64()
JSValue pomelo_qjs_message_write_int64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeFloat32()
JSValue pomelo_qjs_message_write_float32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Message.writeFloat64()
JSValue pomelo_qjs_message_write_float64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_MESSAGE_SRC_H
