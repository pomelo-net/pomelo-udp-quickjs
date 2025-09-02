#include <assert.h>
#include "context.h"
#include "message.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/


// Create prototype for class
static JSCFunctionListEntry message_funcs[] = {
    JS_CFUNC_DEF("reset", 0, pomelo_qjs_message_reset),
    JS_CFUNC_DEF("size", 0, pomelo_qjs_message_size),
    JS_CFUNC_DEF("read", 0, pomelo_qjs_message_read),
    JS_CFUNC_DEF("readUint8", 0, pomelo_qjs_message_read_uint8),
    JS_CFUNC_DEF("readUint16", 0, pomelo_qjs_message_read_uint16),
    JS_CFUNC_DEF("readUint32", 0, pomelo_qjs_message_read_uint32),
    JS_CFUNC_DEF("readUint64", 0, pomelo_qjs_message_read_uint64),
    JS_CFUNC_DEF("readInt8", 0, pomelo_qjs_message_read_int8),
    JS_CFUNC_DEF("readInt16", 0, pomelo_qjs_message_read_int16),
    JS_CFUNC_DEF("readInt32", 0, pomelo_qjs_message_read_int32),
    JS_CFUNC_DEF("readInt64", 0, pomelo_qjs_message_read_int64),
    JS_CFUNC_DEF("readFloat32", 0, pomelo_qjs_message_read_float32),
    JS_CFUNC_DEF("readFloat64", 0, pomelo_qjs_message_read_float64),
    JS_CFUNC_DEF("write", 1, pomelo_qjs_message_write),
    JS_CFUNC_DEF("writeUint8", 1, pomelo_qjs_message_write_uint8),
    JS_CFUNC_DEF("writeUint16", 1, pomelo_qjs_message_write_uint16),
    JS_CFUNC_DEF("writeUint32", 1, pomelo_qjs_message_write_uint32),
    JS_CFUNC_DEF("writeUint64", 1, pomelo_qjs_message_write_uint64),
    JS_CFUNC_DEF("writeInt8", 1, pomelo_qjs_message_write_int8),
    JS_CFUNC_DEF("writeInt16", 1, pomelo_qjs_message_write_int16),
    JS_CFUNC_DEF("writeInt32", 1, pomelo_qjs_message_write_int32),
    JS_CFUNC_DEF("writeInt64", 1, pomelo_qjs_message_write_int64),
    JS_CFUNC_DEF("writeFloat32", 1, pomelo_qjs_message_write_float32),
    JS_CFUNC_DEF("writeFloat64", 1, pomelo_qjs_message_write_float64),
};


int pomelo_qjs_init_message_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    // Generate new class ID
    JSClassID class_id = 0;
    if (JS_NewClassID(context->rt, &class_id) < 0) {
        return -1;
    }
    context->class_message_id = class_id;

    // Register the class
    JSClassDef class_def = {
        .class_name = "Message",
        .finalizer = pomelo_qjs_message_finalizer
    };
    if (JS_NewClass(context->rt, class_id, &class_def) < 0) {
        return -1;
    }
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(
        ctx, proto, message_funcs, countof(message_funcs)
    );

    JSValue message_class = JS_NewCFunction2(
        ctx,
        pomelo_qjs_message_constructor,
        "Message",
        /* argc = */ 0,
        JS_CFUNC_constructor,
        0
    );
    JS_SetConstructor(ctx, message_class, proto);

    // Set the class prototype
    JS_SetClassProto(ctx, class_id, proto);
    JS_SetModuleExport(ctx, m, "Message", message_class);
    return 0;
}


JSValue pomelo_qjs_message_new(
    pomelo_qjs_context_t * context,
    pomelo_message_t * message
) {
    assert(context != NULL);
    assert(message != NULL);

    // Acquire new JS message
    JSValue js_message =
        JS_NewObjectClass(context->ctx, context->class_message_id);
    if (JS_IsException(js_message)) return js_message;

    // Acquire new qjs message
    pomelo_qjs_message_t * qjs_message =
        pomelo_qjs_context_acquire_message(context);
    if (!qjs_message) {
        JS_FreeValue(context->ctx, js_message);
        return JS_NULL;
    }

    // Set opaque value here
    if (JS_SetOpaque(js_message, qjs_message) < 0) {
        JS_FreeValue(context->ctx, js_message);
        pomelo_qjs_context_release_message(context, qjs_message);
        return JS_EXCEPTION; // Failed to set opaque value
    }

    // Set the message
    qjs_message->message = message;
    qjs_message->thiz = js_message; // Just a weak reference

    // Ref the message
    pomelo_message_ref(message);
    return js_message;
}


int pomelo_qjs_message_init(
    pomelo_qjs_message_t * qjs_message,
    pomelo_qjs_context_t * context
) {
    assert(qjs_message != NULL);
    assert(context != NULL);

    qjs_message->context = context;
    qjs_message->message = NULL;
    qjs_message->thiz = JS_NULL;
    return 0;
}


void pomelo_qjs_message_cleanup(pomelo_qjs_message_t * qjs_message) {
    assert(qjs_message != NULL);
    qjs_message->thiz = JS_NULL;
    if (qjs_message->message) {
        pomelo_message_unref(qjs_message->message);
        qjs_message->message = NULL;
    }
}


JSValue pomelo_qjs_send_info_init(
    pomelo_qjs_send_info_t * send_info,
    pomelo_qjs_context_t * context,
    pomelo_qjs_message_t * qjs_message
) {
    assert(send_info != NULL);
    assert(context != NULL);
    assert(qjs_message != NULL);

    send_info->context = context;
    send_info->message = qjs_message->message;
    send_info->js_message = JS_DupValue(context->ctx, qjs_message->thiz);
    pomelo_message_ref(qjs_message->message);

    // The js_message will be freed later in sending callback
    return JS_NewPromiseCapability(context->ctx, send_info->promise_funcs);
}


void pomelo_qjs_send_info_finalize(pomelo_qjs_send_info_t * send_info) {
    assert(send_info != NULL);

    JSContext * ctx = send_info->context->ctx;

    // Unref the message
    if (send_info->message) {
        pomelo_message_unref(send_info->message);
        send_info->message = NULL;
    }

    // Free the js_message
    JS_FreeValue(ctx, send_info->js_message);
    send_info->js_message = JS_NULL;

    // Free functions
    JSValue * funcs = send_info->promise_funcs;
    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);
    funcs[0] = JS_NULL;
    funcs[1] = JS_NULL;

    // Release to pool
    pomelo_qjs_context_release_send_info(send_info->context, send_info);
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

JSValue pomelo_qjs_message_constructor(
    JSContext * ctx, JSValue new_target, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) new_target;
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_message_t * message =
        pomelo_context_acquire_message(context->context);
    if (!message) {
        return JS_ThrowInternalError(ctx, "Failed to acquire message");
    }

    JSValue js_message = pomelo_qjs_message_new(context, message);
    pomelo_message_unref(message);
    return js_message;
}


void pomelo_qjs_message_finalizer(JSRuntime * rt, JSValue val) {
    assert(rt != NULL);
    pomelo_qjs_context_t * context = JS_GetRuntimeOpaque(rt);
    if (!context) return;

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(val, context->class_message_id);
    if (!qjs_message) return;
    
    // Release the message
    pomelo_qjs_context_release_message(context, qjs_message);
}


JSValue pomelo_qjs_message_reset(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) return JS_UNDEFINED;

    pomelo_message_reset(qjs_message->message);
    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_size(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_NewUint32(ctx, 0);
    }


    size_t size = pomelo_message_size(qjs_message->message);
    return JS_NewUint32(ctx, (uint32_t) size);
}


JSValue pomelo_qjs_message_read(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    // Parse length
    size_t length = 0;
    if (JS_IsBigInt(argv[0])) {
        uint64_t temp = 0;
        if (JS_ToBigUint64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        length = (size_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        uint32_t temp = 0;
        if (JS_ToUint32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        length = (size_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }
    
    // There's only one instance of temporary buffer
    uint8_t * buffer = pomelo_qjs_context_prepare_temp_buffer(context, length);
    if (!buffer) {
        return JS_ThrowSyntaxError(ctx, "Failed to prepare temporary buffer");
    }

    int ret = pomelo_message_read_buffer(qjs_message->message, buffer, length);
    if (ret != 0) {
        return JS_ThrowTypeError(ctx, "Message is underflow");
    }

    // Create new Uint8Array with copying
    JSValue result = JS_NewUint8ArrayCopy(ctx, buffer, length);

    // Release the buffer
    pomelo_qjs_context_release_temp_buffer(context, buffer);
    return result;
}


JSValue pomelo_qjs_message_read_uint8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    uint8_t ret = 0;
    int res = pomelo_message_read_uint8(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewUint32(ctx, ret);
}


JSValue pomelo_qjs_message_read_uint16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    uint16_t ret = 0;
    int res = pomelo_message_read_uint16(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewUint32(ctx, ret);
}


JSValue pomelo_qjs_message_read_uint32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    uint32_t ret = 0;
    int res = pomelo_message_read_uint32(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewUint32(ctx, ret);
}


JSValue pomelo_qjs_message_read_uint64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    uint64_t ret = 0;
    int res = pomelo_message_read_uint64(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewBigUint64(ctx, ret);
}


JSValue pomelo_qjs_message_read_int8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int8_t ret = 0;
    int res = pomelo_message_read_int8(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewInt32(ctx, ret);
}


JSValue pomelo_qjs_message_read_int16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int16_t ret = 0;
    int res = pomelo_message_read_int16(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewInt32(ctx, ret);
}


JSValue pomelo_qjs_message_read_int32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int32_t ret = 0;
    int res = pomelo_message_read_int32(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewInt32(ctx, ret);
}


JSValue pomelo_qjs_message_read_int64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int64_t ret = 0;
    int res = pomelo_message_read_int64(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewBigInt64(ctx, ret);
}


JSValue pomelo_qjs_message_read_float32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    float ret = 0.0f;
    int res = pomelo_message_read_float32(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewFloat64(ctx, (double)ret);
}


JSValue pomelo_qjs_message_read_float64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    double ret = 0.0;
    int res = pomelo_message_read_float64(qjs_message->message, &ret);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is underflow");

    return JS_NewFloat64(ctx, ret);
}


JSValue pomelo_qjs_message_write(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);

    // Get the message object
    pomelo_qjs_context_t *context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t *qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowTypeError(ctx, "Invalid native message");
    }

    // Check arguments
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Missing required argument: value");
    }

    // Get the Uint8Array buffer
    size_t length;
    uint8_t * buffer = JS_GetUint8Array(ctx, &length, argv[0]);
    if (!buffer) {
        return JS_ThrowTypeError(ctx, "Expected Uint8Array as first argument");
    }

    // Write the buffer to the message
    int ret = pomelo_message_write_buffer(qjs_message->message, buffer, length);
    if (ret < 0) {
        return JS_ThrowTypeError(ctx, "Message is overflow");
    }

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_uint8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    uint8_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        uint64_t temp = 0;
        if (JS_ToBigUint64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint8_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        uint32_t temp = 0;
        if (JS_ToUint32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint8_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int ret = pomelo_message_write_uint8(qjs_message->message, value);
    if (ret < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_uint16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    uint16_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        uint64_t temp = 0;
        if (JS_ToBigUint64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint16_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        uint32_t temp = 0;
        if (JS_ToUint32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint16_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_uint16(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_uint32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    uint32_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        uint64_t temp = 0;
        if (JS_ToBigUint64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint32_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        if (JS_ToUint32(ctx, &value, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_uint32(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_uint64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    uint64_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        if (JS_ToBigUint64(ctx, &value, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
    } else if (JS_IsNumber(argv[0])) {
        int32_t temp = 0;
        if (JS_ToInt32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (uint64_t)temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_uint64(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_int8(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    int8_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        int64_t temp = 0;
        if (JS_ToBigInt64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int8_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        int32_t temp = 0;
        if (JS_ToInt32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int8_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_int8(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_int16(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    int16_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        int64_t temp = 0;
        if (JS_ToBigInt64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int16_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        int32_t temp = 0;
        if (JS_ToInt32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int16_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_int16(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_int32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    int32_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        int64_t temp = 0;
        if (JS_ToBigInt64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int32_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        if (JS_ToInt32(ctx, &value, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_int32(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_int64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    int64_t value = 0;
    if (JS_IsBigInt(argv[0])) {
        if (JS_ToBigInt64(ctx, &value, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
    } else if (JS_IsNumber(argv[0])) {
        double temp = 0;
        if (JS_ToFloat64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        value = (int64_t)temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_int64(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_float32(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    double value = 0.0;
    if (!JS_IsNumber(argv[0]) || JS_ToFloat64(ctx, &value, argv[0]) != 0) {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_float32(qjs_message->message, (float)value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_message_write_float64(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowSyntaxError(ctx, "Missing argument");
    }

    double value = 0.0;
    if (!JS_IsNumber(argv[0]) || JS_ToFloat64(ctx, &value, argv[0]) != 0) {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_message_t * qjs_message =
        JS_GetOpaque(thiz, context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowSyntaxError(ctx, "Invalid native message");
    }

    int res = pomelo_message_write_float64(qjs_message->message, value);
    if (res < 0) return JS_ThrowTypeError(ctx, "Message is overflow");

    return JS_UNDEFINED;
}
