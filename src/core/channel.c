#include <assert.h>
#include "context.h"
#include "channel.h"
#include "message.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

// JS Channels are managed by native side

static JSCFunctionListEntry channel_funcs[] = {
    JS_CFUNC_DEF("send", 1, pomelo_qjs_channel_send),
    JS_CGETSET_DEF(
        "mode",
        pomelo_qjs_channel_get_mode,
        pomelo_qjs_channel_set_mode
    )
};


int pomelo_qjs_init_channel_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    // Generate new class ID
    JSClassID class_id = 0;
    if (JS_NewClassID(context->rt, &class_id) < 0) {
        return -1;
    }
    context->class_channel_id = class_id;

    // Register the class
    JSClassDef class_def = { .class_name = "Channel" };
    if (JS_NewClass(context->rt, class_id, &class_def) < 0) {
        return -1;
    }

    // Create prototype for class
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(
        ctx, proto, channel_funcs, countof(channel_funcs)
    );

    // Set the class prototype
    JS_SetClassProto(ctx, class_id, proto);

    // No exports
    return 0;
}


JSValue pomelo_qjs_channel_new(
    pomelo_qjs_context_t * context,
    pomelo_channel_t * channel
) {
    assert(context != NULL);
    assert(channel != NULL);
    JSContext * ctx = context->ctx;

    // Create new JS channel
    JSValue js_channel = JS_NewObjectClass(ctx, context->class_channel_id);
    if (JS_IsException(js_channel)) {
        return js_channel; // Failed to create new js channel   
    }
    
    // Acquire new QJS channel
    pomelo_qjs_channel_t * qjs_channel =
        pomelo_qjs_context_acquire_channel(context);
    if (!qjs_channel) {
        JS_FreeValue(ctx, js_channel);
        return JS_EXCEPTION; // Failed to acquire new qjs channel
    }

    // Set the QJS channel to JS channel
    if (JS_SetOpaque(js_channel, qjs_channel) < 0) {
        JS_FreeValue(ctx, js_channel);
        pomelo_qjs_context_release_channel(context, qjs_channel);
        return JS_EXCEPTION;
    }

    qjs_channel->channel = channel;
    qjs_channel->thiz = JS_DupValue(ctx, js_channel);
    pomelo_channel_set_extra(channel, qjs_channel);

    return js_channel;
}


int pomelo_qjs_channel_init(
    pomelo_qjs_channel_t * qjs_channel,
    pomelo_qjs_context_t * context
) {
    assert(qjs_channel != NULL);
    assert(context != NULL);
    qjs_channel->context = context;
    qjs_channel->thiz = JS_NULL;
    return 0;
}


void pomelo_qjs_channel_cleanup(pomelo_qjs_channel_t * qjs_channel) {
    assert(qjs_channel != NULL);
    // Unset the native channel
    if (qjs_channel->channel) {
        pomelo_channel_set_extra(qjs_channel->channel, NULL);
        qjs_channel->channel = NULL;
    }

    // Delete the reference
    if (!JS_IsNull(qjs_channel->thiz)) {
        JS_FreeValue(qjs_channel->context->ctx, qjs_channel->thiz);
        qjs_channel->thiz = JS_NULL;
    }
}


/*----------------------------------------------------------------------------*/
/*                            Implementation APIs                             */
/*----------------------------------------------------------------------------*/

void pomelo_channel_on_cleanup(pomelo_channel_t * channel) {
    assert(channel != NULL);

    pomelo_qjs_channel_t * qjs_channel = pomelo_channel_get_extra(channel);
    if (!qjs_channel) return; // The channel may not be created

    // Release the channel
    pomelo_qjs_context_release_channel(qjs_channel->context, qjs_channel);
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


JSValue pomelo_qjs_channel_set_mode(
    JSContext * ctx, JSValue thiz, JSValue value
) {
    assert(ctx != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_channel_t * qjs_channel =
        JS_GetOpaque(thiz, context->class_channel_id);
    if (!qjs_channel || !qjs_channel->channel) {
        return JS_ThrowTypeError(ctx, "Invalid native channel");
    }

    // Convert value to int
    int mode = 0;
    if (JS_ToInt32(ctx, &mode, value) < 0) {
        return JS_ThrowTypeError(ctx, "mode: Number expected");
    }

    // Check parameter
    if (mode < 0 || mode > 2) {
        return JS_ThrowTypeError(ctx, "mode: Invalid mode");
    }

    // Set the mode
    pomelo_channel_set_mode(qjs_channel->channel, (pomelo_channel_mode) mode);

    return JS_UNDEFINED;
}


JSValue pomelo_qjs_channel_get_mode(JSContext * ctx, JSValue thiz) {
    assert(ctx != NULL);

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_qjs_channel_t * qjs_channel =
        JS_GetOpaque(thiz, context->class_channel_id);
    if (!qjs_channel || !qjs_channel->channel) {
        return JS_ThrowTypeError(ctx, "mode: Invalid native channel");
    }

    // Get the mode
    pomelo_channel_mode mode = pomelo_channel_get_mode(qjs_channel->channel);
    return JS_NewInt32(ctx, (int32_t) mode);
}


JSValue pomelo_qjs_channel_send(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "send: At least one argument expected");
    }

    // Check if value is object
    if (!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "send: Object expected");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    // Get the qjs channel
    pomelo_qjs_channel_t * qjs_channel =
        JS_GetOpaque(thiz, context->class_channel_id);
    if (!qjs_channel || !qjs_channel->channel) {
        return JS_ThrowTypeError(ctx, "send: Invalid native channel");
    }

    // Get the qjs message
    pomelo_qjs_message_t * qjs_message = 
        JS_GetOpaque(argv[0], context->class_message_id);
    if (!qjs_message || !qjs_message->message) {
        return JS_ThrowTypeError(ctx, "send: Message expected");
    }

    // Create new promise
    pomelo_qjs_send_info_t * send_info =
        pomelo_qjs_context_acquire_send_info(context);
    if (!send_info) {
        return JS_ThrowTypeError(ctx, "send: Failed to acquire send info");
    }
    JSValue promise =
        pomelo_qjs_send_info_init(send_info, context, qjs_message);
    if (JS_IsException(promise)) {
        pomelo_qjs_context_release_send_info(context, send_info);
        return promise;
    }

    // Send the message
    pomelo_channel_send(qjs_channel->channel, qjs_message->message, send_info);
    return promise;
}
