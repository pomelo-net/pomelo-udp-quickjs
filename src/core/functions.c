#include <assert.h>
#include "functions.h"
#include "context.h"


int pomelo_qjs_init_global_functions(JSContext * ctx, JSModuleDef * m) {
    // Create statistic function
    JSValue fn_statistic = JS_NewCFunction(
        ctx,
        pomelo_qjs_context_statistic,
        "statistic",
        /* argc = */ 0
    );
    JS_SetModuleExport(ctx, m, "statistic", fn_statistic);
    return 0;
}


/*----------------------------------------------------------------------------*/
/*                              Statistic APIs                                */
/*----------------------------------------------------------------------------*/


JSValue pomelo_qjs_context_statistic(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) argc;
    (void) thiz;
    (void) argv;

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    if (!context) return JS_ThrowTypeError(ctx, "Invalid context");

    pomelo_statistic_t statistic = {0};
    pomelo_context_statistic(context->context, &statistic);

    // Create the root statistic object
    JSValue js_statistic = JS_NewObject(ctx);

    // Add allocator stats
    JSValue js_allocator = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx, 
        js_allocator, 
        "allocatedBytes", 
        JS_NewBigUint64(ctx, statistic.allocator.allocated_bytes)
    );
    JS_SetPropertyStr(ctx, js_statistic, "allocator", js_allocator);

    // Add API stats
    JSValue js_api = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx, js_api, "messages", 
        JS_NewUint32(ctx, (uint32_t) statistic.api.messages)
    );
    JS_SetPropertyStr(
        ctx, js_api, "builtinSessions", 
        JS_NewUint32(ctx, (uint32_t) statistic.api.builtin_sessions)
    );
    JS_SetPropertyStr(
        ctx, js_api, "pluginSessions", 
        JS_NewUint32(ctx, (uint32_t) statistic.api.plugin_sessions)
    );
    JS_SetPropertyStr(
        ctx, js_api, "builtinChannels", 
        JS_NewUint32(ctx, (uint32_t) statistic.api.builtin_channels)
    );
    JS_SetPropertyStr(
        ctx, js_api, "pluginChannels", 
        JS_NewUint32(ctx, (uint32_t) statistic.api.plugin_channels)
    );
    JS_SetPropertyStr(
        ctx, js_api, "sessionDisconnectRequests", 
        JS_NewUint32(ctx, 0) // Not available in C struct
    );
    JS_SetPropertyStr(ctx, js_statistic, "api", js_api);

    // Add buffer stats
    JSValue js_buffer = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx, js_buffer, "buffers", 
        JS_NewUint32(ctx, (uint32_t) statistic.buffer.buffers)
    );
    JS_SetPropertyStr(ctx, js_statistic, "buffer", js_buffer);

    // Add protocol stats
    JSValue js_protocol = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx, js_protocol, "senders", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.senders)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "receivers", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.receivers)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "packets", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.packets)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "peers", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.peers)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "servers", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.servers)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "clients", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.clients)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "cryptoContexts", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.crypto_contexts)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "acceptances", 
        JS_NewUint32(ctx, (uint32_t) statistic.protocol.acceptances)
    );
    JS_SetPropertyStr(
        ctx, js_protocol, "peerDisconnectRequests", 
        JS_NewUint32(ctx, 0) // Not available in C struct
    );
    JS_SetPropertyStr(ctx, js_statistic, "protocol", js_protocol);

    // Add delivery stats
    JSValue js_delivery = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "dispatchers", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.dispatchers)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "senders", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.senders)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "receivers", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.receivers)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "endpoints", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.endpoints)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "buses", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.buses)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "parcels", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.parcels)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "receptions", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.receptions)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "transmissions", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.transmissions)
    );
    JS_SetPropertyStr(
        ctx,
        js_delivery,
        "heartbeats", 
        JS_NewUint32(ctx, (uint32_t) statistic.delivery.heartbeats)
    );
    JS_SetPropertyStr(ctx, js_statistic, "delivery", js_delivery);

    // Add binding stats (not available in C struct)
    JSValue js_binding = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx,
        js_binding,
        "messages", 
        JS_NewUint32(ctx, (uint32_t) pomelo_pool_in_use(context->pool_message))
    );
    JS_SetPropertyStr(
        ctx,
        js_binding,
        "sockets", 
        JS_NewUint32(ctx, (uint32_t) pomelo_pool_in_use(context->pool_socket))
    );
    JS_SetPropertyStr(
        ctx,
        js_binding,
        "sessions",
        JS_NewUint32(ctx, (uint32_t) pomelo_pool_in_use(context->pool_session))
    );
    JS_SetPropertyStr(
        ctx,
        js_binding,
        "channels",
        JS_NewUint32(ctx, (uint32_t) pomelo_pool_in_use(context->pool_channel))
    );
    JS_SetPropertyStr(ctx, js_statistic, "binding", js_binding);

    // Platform statistic
    JSValue platform_statistic =
        pomelo_qjs_platform_statistic(ctx, context->platform);
    
    JS_SetPropertyStr(ctx, js_statistic, "platform", platform_statistic);
    return js_statistic;
}
