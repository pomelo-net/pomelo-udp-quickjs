#include <assert.h>
#include <string.h>
#include "pomelo/base64.h"
#include "pomelo/constants.h"
#include "socket.h"
#include "message.h"
#include "context.h"
#include "session.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))


#define POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH                              \
    pomelo_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES)



static JSCFunctionListEntry socket_funcs[] = {
    JS_CFUNC_DEF("setListener", 1, pomelo_qjs_socket_set_listener),
    JS_CFUNC_DEF("listen", 4, pomelo_qjs_socket_listen),
    JS_CFUNC_DEF("connect", 1, pomelo_qjs_socket_connect),
    JS_CFUNC_DEF("stop", 0, pomelo_qjs_socket_stop),
    JS_CFUNC_DEF("send", 3, pomelo_qjs_socket_send),
    JS_CFUNC_DEF("time", 0, pomelo_qjs_socket_time),
};


int pomelo_qjs_init_socket_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    // Generate new class ID
    JSClassID class_id = 0;
    if (JS_NewClassID(context->rt, &class_id) < 0) {
        return -1;
    }
    context->class_socket_id = class_id;

    // Register the class
    JSClassDef class_def = {
        .class_name = "Socket",
        .finalizer = pomelo_qjs_socket_finalizer
    };
    if (JS_NewClass(context->rt, class_id, &class_def) < 0) {
        return -1;
    }

    // Create prototype for class
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(
        ctx, proto, socket_funcs, countof(socket_funcs)
    );

    JSValue socket_class = JS_NewCFunction2(
        ctx, pomelo_qjs_socket_constructor, "Socket", 1, JS_CFUNC_constructor, 0
    );
    JS_SetConstructor(ctx, socket_class, proto);

    // Set the class prototype
    JS_SetClassProto(ctx, class_id, proto);
    JS_SetModuleExport(ctx, m, "Socket", socket_class);
    return 0;
}


int pomelo_qjs_socket_init(
    pomelo_qjs_socket_t * qjs_socket,
    pomelo_qjs_context_t * context
) {
    assert(qjs_socket != NULL);
    assert(context != NULL);
    
    qjs_socket->context = context;
    qjs_socket->socket = NULL;
    qjs_socket->thiz = JS_NULL;
    qjs_socket->listener = JS_NULL;
    qjs_socket->on_connected = JS_NULL;
    qjs_socket->on_disconnected = JS_NULL;
    qjs_socket->on_received = JS_NULL;
    qjs_socket->connect_callback_funcs[0] = JS_NULL;
    qjs_socket->connect_callback_funcs[1] = JS_NULL;
    qjs_socket->thiz_entry = NULL;

    return 0;
}


void pomelo_qjs_socket_cleanup(pomelo_qjs_socket_t * qjs_socket) {
    assert(qjs_socket != NULL);

    JSContext * ctx = qjs_socket->context->ctx;
    
    qjs_socket->context = NULL;
    if (qjs_socket->socket) {
        pomelo_socket_destroy(qjs_socket->socket);
        qjs_socket->socket = NULL;
    }

    // Do not free thiz here. JS socket object is controlling native socket.
    qjs_socket->thiz = JS_NULL;

    JS_FreeValue(ctx, qjs_socket->listener);
    qjs_socket->listener = JS_NULL;
    
    JS_FreeValue(ctx, qjs_socket->on_connected);
    qjs_socket->on_connected = JS_NULL;
    
    JS_FreeValue(ctx, qjs_socket->on_disconnected);
    qjs_socket->on_disconnected = JS_NULL;

    JS_FreeValue(ctx, qjs_socket->on_received);
    qjs_socket->on_received = JS_NULL;

    JS_FreeValue(ctx, qjs_socket->connect_callback_funcs[0]);
    qjs_socket->connect_callback_funcs[0] = JS_NULL;

    JS_FreeValue(ctx, qjs_socket->connect_callback_funcs[1]);
    qjs_socket->connect_callback_funcs[1] = JS_NULL;
}

/*----------------------------------------------------------------------------*/
/*                            Implementation APIs                             */
/*----------------------------------------------------------------------------*/

void pomelo_socket_on_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    assert(socket != NULL);
    assert(session != NULL);

    pomelo_qjs_socket_t * qjs_socket = pomelo_socket_get_extra(socket);
    if (!qjs_socket) return; // No associated socket

    // Call the callback
    JSContext * ctx = qjs_socket->context->ctx;
    JSValue on_connected = qjs_socket->on_connected;
    JSValue listener = qjs_socket->listener;
    if (!JS_IsFunction(ctx, qjs_socket->on_connected)) return;
    
    JSValue js_session =
        pomelo_qjs_session_new(qjs_socket->context, session);
    JSValue ret = JS_Call(ctx, on_connected, listener, 1, &js_session);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, js_session);
}


void pomelo_socket_on_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    assert(socket != NULL);
    assert(session != NULL);

    pomelo_qjs_socket_t * qjs_socket = pomelo_socket_get_extra(socket);
    if (!qjs_socket) return; // No associated socket

    // Call the callback
    JSContext * ctx = qjs_socket->context->ctx;
    JSValue on_disconnected = qjs_socket->on_disconnected;
    JSValue listener = qjs_socket->listener;
    if (!JS_IsFunction(ctx, qjs_socket->on_disconnected)) return;

    pomelo_qjs_session_t * qjs_session = pomelo_session_get_extra(session);
    if (!qjs_session) return; // No associated session

    JSValue js_session = qjs_session->thiz;
    JSValue ret = JS_Call(ctx, on_disconnected, listener, 1, &js_session);
    JS_FreeValue(ctx, ret);
}


void pomelo_socket_on_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    assert(socket != NULL);
    assert(session != NULL);
    assert(message != NULL);

    pomelo_qjs_socket_t * qjs_socket = pomelo_socket_get_extra(socket);
    if (!qjs_socket) return; // No associated socket

    // Call the callback
    JSContext * ctx = qjs_socket->context->ctx;
    JSValue on_received = qjs_socket->on_received;
    JSValue listener = qjs_socket->listener;
    if (!JS_IsFunction(ctx, qjs_socket->on_received)) return;

    pomelo_qjs_session_t * qjs_session = pomelo_session_get_extra(session);
    if (!qjs_session) return; // No associated session

    JSValue js_session = qjs_session->thiz;
    
    // Wrap the native message to JS message
    JSValue js_message = pomelo_qjs_message_new(qjs_socket->context, message);
    JSValue args[] = { js_session, js_message };
    JSValue ret = JS_Call(ctx, on_received, listener, countof(args), args);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, js_message);
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    assert(socket != NULL);
    (void) result;

    pomelo_qjs_socket_t * qjs_socket = pomelo_socket_get_extra(socket);
    if (!qjs_socket) return; // No associated socket

    JSContext * ctx = qjs_socket->context->ctx;
    JSValue * funcs = qjs_socket->connect_callback_funcs;

    // Resolve
    if (JS_IsFunction(ctx, funcs[0])) {
        JSValue connect_result = JS_NewInt32(ctx, result);
        JSValue ret = JS_Call(ctx, funcs[0], JS_NULL, 1, &connect_result);
        JS_FreeValue(ctx, ret);
        JS_FreeValue(ctx, connect_result);
    }

    // Free the callback functions
    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);

    funcs[0] = JS_NULL;
    funcs[1] = JS_NULL;
}


void pomelo_socket_on_send_result(
    pomelo_socket_t * socket,
    pomelo_message_t * message,
    void * data,
    size_t send_count
) {
    assert(socket != NULL);
    assert(message != NULL);
    pomelo_qjs_send_info_t * send_info = (pomelo_qjs_send_info_t *) data;
    JSContext * ctx = send_info->context->ctx;

    // Call the callback
    JSValue count = JS_NewUint32(ctx, (uint32_t) send_count);
    JSValue ret = JS_Call(ctx, send_info->promise_funcs[0], JS_NULL, 1, &count);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, count);

    pomelo_qjs_send_info_finalize(send_info);
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


JSValue pomelo_qjs_socket_constructor(
    JSContext * ctx, JSValue new_target, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) new_target;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    if (argc < 1) return JS_ThrowTypeError(ctx, "Missing channel modes");

    if (!JS_IsArray(argv[0])) {
        return JS_ThrowTypeError(ctx, "Channel modes must be an array");
    }

    int64_t nchannels = 0;
    if (JS_GetLength(ctx, argv[0], &nchannels) != 0) {
        return JS_ThrowTypeError(ctx, "Failed to get channel modes length");
    }
    pomelo_channel_mode modes[POMELO_MAX_CHANNELS];

    for (int64_t i = 0; i < nchannels; i++) {
        JSValue mode = JS_GetPropertyInt64(ctx, argv[0], i);
        int32_t mode_int = 0;
        if (JS_ToInt32(ctx, &mode_int, mode) != 0) {
            JS_FreeValue(ctx, mode);
            return JS_ThrowTypeError(ctx, "Channel mode must be a number");
        }
        modes[i] = (pomelo_channel_mode) mode_int;
        JS_FreeValue(ctx, mode);
    }

    // Create new js socket object
    JSValue thiz = JS_NewObjectClass(ctx, context->class_socket_id);
    if (JS_IsException(thiz)) return thiz;

    // Acquire new qjs_socket
    pomelo_qjs_socket_t * qjs_socket =
        pomelo_qjs_context_acquire_socket(context);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Failed to acquire socket");

    // Set the qjs_socket to thiz
    if (JS_SetOpaque(thiz, qjs_socket) < 0) {
        JS_FreeValue(ctx, thiz);
        pomelo_qjs_context_release_socket(context, qjs_socket);
        return JS_ThrowTypeError(ctx, "Failed to set opaque socket");
    }

    // Do not dup value thiz here.
    // It will be dupplicated later when listening or connecting

    // Create new socket
    pomelo_socket_options_t options = {
        .context = context->context,
        .platform = context->platform,
        .nchannels = (size_t) nchannels,
        .channel_modes = modes,
    };
    pomelo_socket_t * socket = pomelo_socket_create(&options);
    if (!socket) {
        pomelo_qjs_context_release_socket(context, qjs_socket);
        JS_FreeValue(ctx, thiz);
        return JS_ThrowTypeError(ctx, "Failed to create socket");
    }

    // Set the socket to qjs_socket
    qjs_socket->socket = socket;
    pomelo_socket_set_extra(socket, qjs_socket);

    // Just a weak ref
    qjs_socket->thiz = thiz;

    return thiz;
}


void pomelo_qjs_socket_finalizer(JSRuntime * rt, JSValue val) {
    assert(rt != NULL);
    pomelo_qjs_context_t * context = JS_GetRuntimeOpaque(rt);
    if (!context) return;

    pomelo_qjs_socket_t * qjs_socket = JS_GetOpaque(val, context->class_socket_id);
    if (!qjs_socket) return;
    
    // Release the socket
    pomelo_qjs_context_release_socket(context, qjs_socket);
}


JSValue pomelo_qjs_socket_set_listener(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) return JS_ThrowTypeError(ctx, "Missing listener");

    if (!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Listener must be an object");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Invalid native socket");

    // If listener is already set, free the old one
    JS_FreeValue(ctx, qjs_socket->listener);
    JS_FreeValue(ctx, qjs_socket->on_connected);
    JS_FreeValue(ctx, qjs_socket->on_disconnected);
    JS_FreeValue(ctx, qjs_socket->on_received);

    qjs_socket->listener = JS_DupValue(ctx, argv[0]);
    qjs_socket->on_connected = JS_GetPropertyStr(ctx, argv[0], "onConnected");
    qjs_socket->on_disconnected = JS_GetPropertyStr(ctx, argv[0], "onDisconnected");
    qjs_socket->on_received = JS_GetPropertyStr(ctx, argv[0], "onReceived");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_socket_listen(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 4) return JS_ThrowTypeError(ctx, "Missing arguments");
    
    if (!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Private key must be an object");
    }

    if (!JS_IsNumber(argv[1]) && !JS_IsBigInt(argv[1])) {
        return JS_ThrowTypeError(ctx, "Protocol ID must be a number or bigint");
    }

    if (!JS_IsNumber(argv[2])) {
        return JS_ThrowTypeError(ctx, "Max clients must be a number");
    }

    if (!JS_IsString(argv[3])) {
        return JS_ThrowTypeError(ctx, "Address must be a string");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Invalid native socket");

    // Get private key
    size_t length = 0;
    uint8_t * data = JS_GetUint8Array(ctx, &length, argv[0]);
    if (!data || length != POMELO_KEY_BYTES) {
        return JS_ThrowTypeError(ctx, "Invalid private key");
    }
    uint8_t private_key[POMELO_KEY_BYTES];
    memcpy(private_key, data, POMELO_KEY_BYTES);

    // Get protocol ID
    int64_t protocol_id = 0;
    if (JS_IsBigInt(argv[1])) {
        if (JS_ToBigInt64(ctx, &protocol_id, argv[1]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid protocol ID");
        }
    } else {
        if (JS_ToInt64(ctx, &protocol_id, argv[1]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid protocol ID");
        }
    }

    // Get max clients
    int32_t max_clients = 0;
    if (JS_ToInt32(ctx, &max_clients, argv[2]) != 0) {
        return JS_ThrowTypeError(ctx, "Invalid max clients");
    }

    // Get address
    const char * address_str = JS_ToCString(ctx, argv[3]);
    if (!address_str) {
        return JS_ThrowTypeError(ctx, "Invalid address");
    }

    pomelo_address_t address;
    int parse_ret = pomelo_address_from_string(&address, address_str);
    JS_FreeCString(ctx, address_str);
    if (parse_ret < 0) {
        return JS_ThrowTypeError(ctx, "Invalid address");
    }
    
    int ret = pomelo_socket_listen(
        qjs_socket->socket,
        private_key,
        protocol_id,
        max_clients,
        &address
    );

    // Create promise to return
    JSValue funcs[2] = { JS_NULL };
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);

    if (ret < 0) {
        JSValue error = JS_NewError(ctx);
        JS_DefinePropertyValueStr(
            ctx,
            error,
            "message",
            JS_NewString(ctx, "Failed to listen"),
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE
        );
        JSValue call_ret = JS_Call(ctx, funcs[1], JS_NULL, 1, &error);
        JS_FreeValue(ctx, call_ret);
        JS_FreeValue(ctx, error);
    } else {
        JSValue call_ret = JS_Call(ctx, funcs[0], JS_NULL, 0, NULL);
        JS_FreeValue(ctx, call_ret);

        // Make thiz a strong reference
        qjs_socket->thiz = JS_DupValue(ctx, thiz);
        qjs_socket->thiz_entry = pomelo_list_push_back(
            context->running_sockets,
            qjs_socket
        );
    }

    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);
    return promise;
}


JSValue pomelo_qjs_socket_connect(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) return JS_ThrowTypeError(ctx, "Missing connect token");

    if (!JS_IsObject(argv[0]) && !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(
            ctx, "Connect token must be an Uint8Array or string"
        );
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Invalid native socket");

    // Temp array to store the decoded token from string
    uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];

    if (JS_IsString(argv[0])) {
        const char * b64 = JS_ToCString(ctx, argv[0]);
        if (!b64) return JS_ThrowTypeError(ctx, "Invalid connect token");

        size_t b64_bytes = strlen(b64);
        if (b64_bytes != (POMELO_CONNECT_TOKEN_BASE64_BUFFER_LENGTH - 1)) {
            JS_FreeCString(ctx, b64);
            return JS_ThrowTypeError(
                ctx, "Invalid base64 connect token length"
            );
        }

        int ret = pomelo_base64_decode(
            connect_token,
            POMELO_CONNECT_TOKEN_BYTES,
            b64,
            b64_bytes
        );
        JS_FreeCString(ctx, b64);

        if (ret < 0) {
            return JS_ThrowTypeError(
                ctx, "Failed to decode base64 connect token"
            );
        }
    } else {
        // Get the raw pointer from buffer
        size_t length = 0;
        uint8_t * data = JS_GetUint8Array(ctx, &length, argv[0]);

        if (!data || length != POMELO_CONNECT_TOKEN_BYTES) {
            return JS_ThrowTypeError(ctx, "Invalid connect token");
        }

        memcpy(connect_token, data, POMELO_CONNECT_TOKEN_BYTES);
    }

    int ret = pomelo_socket_connect(qjs_socket->socket, connect_token);

    // Create promise to return
    JSValue funcs[2] = { JS_NULL };
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);

    if (ret < 0) {
        JSValue error = JS_NewError(ctx);
        JS_DefinePropertyValueStr(
            ctx,
            error,
            "message",
            JS_NewString(ctx, "Failed to connect"),
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE
        );
        JSValue call_ret = JS_Call(ctx, funcs[1], JS_NULL, 1, &error);
        JS_FreeValue(ctx, call_ret);
        JS_FreeValue(ctx, error);
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    // Make thiz a strong reference
    qjs_socket->thiz = JS_DupValue(ctx, thiz);
    qjs_socket->thiz_entry = pomelo_list_push_back(
        context->running_sockets,
        qjs_socket
    );

    qjs_socket->connect_callback_funcs[0] = funcs[0];
    qjs_socket->connect_callback_funcs[1] = funcs[1];

    return promise;
}


JSValue pomelo_qjs_socket_stop(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Invalid native socket");

    pomelo_qjs_socket_stop_impl(qjs_socket);
    return JS_UNDEFINED;
}


JSValue pomelo_qjs_socket_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 3) return JS_ThrowTypeError(ctx, "Missing arguments");

    // Get context
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    // Get channel index
    uint32_t channel_index = 0;
    if (JS_ToUint32(ctx, &channel_index, argv[0]) != 0) {
        return JS_ThrowTypeError(ctx, "Channel index must be a number");
    }

    // Get message
    if (!JS_IsObject(argv[1])) {
        return JS_ThrowTypeError(ctx, "Message must be an object");
    }

    pomelo_qjs_message_t * qjs_message = JS_GetOpaque(
        argv[1], context->class_message_id
    );
    if (!qjs_message) {
        return JS_ThrowTypeError(ctx, "Invalid native message");
    }

    // Get recipients
    if (!JS_IsArray(argv[2])) {
        return JS_ThrowTypeError(ctx, "Recipients must be an array");
    }

    // Get recipients length
    int64_t recipients_length = 0;
    if (JS_GetLength(ctx, argv[2], &recipients_length) != 0) {
        return JS_ThrowTypeError(ctx, "Failed to get recipients length");
    }

    // Get QJS socket
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) return JS_ThrowTypeError(ctx, "Invalid native socket");

    // Get recipients
    pomelo_array_t * send_sessions = context->tmp_send_sessions;
    int ret = pomelo_array_resize(send_sessions, recipients_length);
    if (ret < 0) {
        return JS_ThrowTypeError(ctx, "Failed to resize recipients");
    }

    size_t array_index = 0;
    for (int64_t i = 0; i < recipients_length; i++) {
        // Get recipient
        JSValue recipient = JS_GetPropertyInt64(ctx, argv[2], i);
        pomelo_qjs_session_t * qjs_session =
            JS_GetOpaque(recipient, context->class_session_id);
        JS_FreeValue(ctx, recipient);

        if (!qjs_session || !qjs_session->session) continue;
        pomelo_array_set(send_sessions, array_index, qjs_session->session);
        array_index++;
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

    pomelo_socket_send(
        qjs_socket->socket,
        channel_index,
        qjs_message->message,
        send_sessions->elements,
        send_sessions->size,
        send_info
    );

    return promise;
}


JSValue pomelo_qjs_socket_time(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);
    
    pomelo_qjs_socket_t * qjs_socket =
        JS_GetOpaque(thiz, context->class_socket_id);
    if (!qjs_socket) {
        return JS_ThrowTypeError(ctx, "Invalid native socket");
    }

    return JS_NewBigUint64(ctx, pomelo_socket_time(qjs_socket->socket));
}


void pomelo_qjs_socket_stop_impl(pomelo_qjs_socket_t * qjs_socket) {
    assert(qjs_socket != NULL);
    pomelo_qjs_context_t * context = qjs_socket->context;
    pomelo_socket_t * socket = qjs_socket->socket;
    
    pomelo_socket_state state = pomelo_socket_get_state(socket);
    if (state == POMELO_SOCKET_STATE_STOPPED) return; // Already stopped

    // Stop the socket
    pomelo_socket_stop(socket);

    // Free the thiz reference
    JS_FreeValue(context->ctx, qjs_socket->thiz);

    if (qjs_socket->thiz_entry) {
        pomelo_list_remove(context->running_sockets, qjs_socket->thiz_entry);
        qjs_socket->thiz_entry = NULL;
    }
}
