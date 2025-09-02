#include <assert.h>
#include "context.h"
#include "session.h"
#include "message.h"
#include "channel.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

// JS Sessions are managed by native side


static JSCFunctionListEntry session_funcs[] = {
    JS_CFUNC_DEF("send", 2, pomelo_qjs_session_send),
    JS_CGETSET_DEF("id", pomelo_qjs_session_get_id, NULL),
    JS_CFUNC_DEF("disconnect", 0, pomelo_qjs_session_disconnect),
    JS_CFUNC_DEF("setChannelMode", 2, pomelo_qjs_session_set_channel_mode),
    JS_CFUNC_DEF("getChannelMode", 1, pomelo_qjs_session_get_channel_mode),
    JS_CFUNC_DEF("rtt", 0, pomelo_qjs_session_rtt),
    JS_CGETSET_DEF("channels", pomelo_qjs_session_get_channels, NULL)
};

int pomelo_qjs_init_session_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    // Generate new class ID
    JSClassID class_id = 0;
    if (JS_NewClassID(context->rt, &class_id) < 0) {
        return -1;
    }
    context->class_session_id = class_id;

    // Register the class
    JSClassDef class_def = { .class_name = "Session" };
    if (JS_NewClass(context->rt, class_id, &class_def) < 0) {
        return -1;
    }

    // Create prototype for class
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(
        ctx, proto, session_funcs, countof(session_funcs)
    );

    // Set the class prototype
    JS_SetClassProto(ctx, class_id, proto);

    // No exports
    return 0;
}


JSValue pomelo_qjs_session_new(
    pomelo_qjs_context_t * context,
    pomelo_session_t * session
) {
    assert(context != NULL);
    assert(session != NULL);
    JSContext * ctx = context->ctx;

    // Create new JS session
    JSValue js_session = JS_NewObjectClass(ctx, context->class_session_id);
    if (JS_IsException(js_session)) {
        return js_session; // Failed to create new js session   
    }
    
    // Acquire new QJS session
    pomelo_qjs_session_t * qjs_session =
        pomelo_qjs_context_acquire_session(context);
    if (!qjs_session) {
        JS_FreeValue(ctx, js_session);
        return JS_EXCEPTION; // Failed to acquire new qjs session
    }

    // Set the QJS session to JS session
    if (JS_SetOpaque(js_session, qjs_session) < 0) {
        JS_FreeValue(ctx, js_session);
        pomelo_qjs_context_release_session(context, qjs_session);
        return JS_EXCEPTION;
    }

    // Set the session
    qjs_session->session = session;
    qjs_session->thiz = JS_DupValue(ctx, js_session);
    pomelo_session_set_extra(session, qjs_session);

    return js_session;
}


int pomelo_qjs_session_init(
    pomelo_qjs_session_t * qjs_session,
    pomelo_qjs_context_t * context
) {
    assert(qjs_session != NULL);
    assert(context != NULL);

    qjs_session->context = context;
    qjs_session->thiz = JS_NULL;
    qjs_session->channels = JS_NULL;

    return 0;
}


void pomelo_qjs_session_cleanup(pomelo_qjs_session_t * qjs_session) {
    assert(qjs_session != NULL);
    assert(qjs_session->context != NULL);

    JSContext * ctx = qjs_session->context->ctx;

    // Unset the native session
    if (qjs_session->session) {
        pomelo_session_set_extra(qjs_session->session, NULL);
        qjs_session->session = NULL;
    }

    // Delete the reference of thiz
    JS_FreeValue(ctx, qjs_session->thiz);
    qjs_session->thiz = JS_NULL;

    // Delete the reference of channels
    JS_FreeValue(ctx, qjs_session->channels);
    qjs_session->channels = JS_NULL;
}


/*----------------------------------------------------------------------------*/
/*                            Implementation APIs                             */
/*----------------------------------------------------------------------------*/

void pomelo_session_on_cleanup(pomelo_session_t * session) {
    assert(session != NULL);

    pomelo_qjs_session_t * qjs_session = pomelo_session_get_extra(session);
    if (!qjs_session) return; // The session may not be created

    pomelo_qjs_context_release_session(qjs_session->context, qjs_session);
}


JSValue pomelo_qjs_session_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "Missing arguments"); 
    }

    // Get the channel index
    int channel_index = 0;
    if (JS_ToInt32(ctx, &channel_index, argv[0]) < 0) {
        return JS_ThrowTypeError(ctx, "Invalid channelIndex");
    }

    // Get the message from the second argument
    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(argv[1], context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowTypeError(ctx, "Invalid native message");
    }

    // Acquire new send info
    pomelo_qjs_send_info_t * send_info =
        pomelo_qjs_context_acquire_send_info(context);
    if (!send_info) {
        return JS_ThrowTypeError(ctx, "Failed to acquire send info");
    }

    // Initialize the send info
    JSValue promise =
        pomelo_qjs_send_info_init(send_info, context, qjs_message);
    if (JS_IsException(promise)) {
        pomelo_qjs_context_release_send_info(context, send_info);
        return promise;
    }

    // Send the message
    pomelo_session_send(
        qjs_session->session,
        (size_t) channel_index,
        qjs_message->message,
        send_info
    );
    
    return promise;
}


JSValue pomelo_qjs_session_get_id(JSContext * ctx, JSValue thiz) {
    assert(ctx != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    int64_t client_id = pomelo_session_get_client_id(qjs_session->session);
    return JS_NewBigInt64(ctx, client_id);
}


JSValue pomelo_qjs_session_disconnect(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    int ret = pomelo_session_disconnect(qjs_session->session);
    return JS_NewBool(ctx, ret == 0);
}


JSValue pomelo_qjs_session_set_channel_mode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "Missing arguments");
    }

    int channel_index = 0;
    if (JS_ToInt32(ctx, &channel_index, argv[0]) < 0) {
        return JS_ThrowTypeError(ctx, "Invalid channelIndex");
    }

    int mode = 0;
    if (JS_ToInt32(ctx, &mode, argv[1]) < 0) {
        return JS_ThrowTypeError(ctx, "Invalid mode");
    }

    if (mode < 0 || mode > 2) {
        return JS_ThrowTypeError(ctx, "Invalid mode");
    }

    int ret = pomelo_session_set_channel_mode(
        qjs_session->session,
        (size_t) channel_index,
        (pomelo_channel_mode) mode
    );

    return JS_NewBool(ctx, ret == 0);
}


JSValue pomelo_qjs_session_get_channel_mode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Missing arguments");
    }

    int channel_index = 0;
    if (JS_ToInt32(ctx, &channel_index, argv[0]) < 0) {
        return JS_ThrowTypeError(ctx, "Invalid channelIndex");
    }

    pomelo_channel_mode mode = pomelo_session_get_channel_mode(
        qjs_session->session,
        (size_t) channel_index
    );

    return JS_NewInt32(ctx, (int32_t) mode);
}


JSValue pomelo_qjs_session_rtt(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    pomelo_rtt_t rtt;
    int ret = pomelo_session_get_rtt(qjs_session->session, &rtt);
    if (ret < 0) return JS_EXCEPTION;

    JSValue result = JS_NewObject(ctx);
    JSValue mean = JS_NewBigUint64(ctx, rtt.mean);
    JSValue variance = JS_NewBigUint64(ctx, rtt.variance);

    JS_SetPropertyStr(ctx, result, "mean", mean);
    JS_SetPropertyStr(ctx, result, "variance", variance);
    
    return result;
}


JSValue pomelo_qjs_session_get_channels(JSContext * ctx, JSValue thiz) {
    assert(ctx != NULL);
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_session_t * qjs_session =
        JS_GetOpaque(thiz, context->class_session_id);
    if (!qjs_session || !qjs_session->session) {
        return JS_ThrowTypeError(ctx, "Invalid native session");
    }

    // If channels array was created, return it
    if (!JS_IsNull(qjs_session->channels)) {
        return JS_DupValue(ctx, qjs_session->channels);
    }

    pomelo_session_t * session = qjs_session->session;
    pomelo_socket_t * socket = pomelo_session_get_socket(session);
    size_t nchannels = pomelo_socket_get_nchannels(socket);

    // Create new array of channels
    JSValue js_channels = JS_NewArray(ctx);
    qjs_session->channels = js_channels;

    for (size_t i = 0; i < nchannels; i++) {
        pomelo_channel_t * channel = pomelo_session_get_channel(session, i);
        assert(channel != NULL);
        JSValue js_channel = pomelo_qjs_channel_new(context, channel);
        if (JS_IsException(js_channel)) {
            return js_channel;
        }
        JS_SetPropertyUint32(ctx, js_channels, (uint32_t) i, js_channel);
    }

    return JS_DupValue(ctx, js_channels);
}
