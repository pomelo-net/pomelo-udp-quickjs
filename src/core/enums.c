#include "enums.h"
#include "pomelo/api.h"


int pomelo_qjs_init_enums(JSContext * ctx, JSModuleDef * m) {
    // Create object ChannelMode
    JSValue channel_mode = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx,
        channel_mode,
        "UNRELIABLE",
        JS_NewInt32(ctx, POMELO_CHANNEL_MODE_UNRELIABLE)
    );

    JS_SetPropertyStr(
        ctx,
        channel_mode,
        "SEQUENCED",
        JS_NewInt32(ctx, POMELO_CHANNEL_MODE_SEQUENCED)
    );

    JS_SetPropertyStr(
        ctx,
        channel_mode,
        "RELIABLE",
        JS_NewInt32(ctx, POMELO_CHANNEL_MODE_RELIABLE)
    );
    JS_SetModuleExport(ctx, m, "ChannelMode", channel_mode);

    // Create object ConnectResult
    JSValue connect_result = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx,
        connect_result,
        "SUCCESS",
        JS_NewInt32(ctx, POMELO_SOCKET_CONNECT_SUCCESS)
    );

    JS_SetPropertyStr(
        ctx,
        connect_result,
        "DENIED",
        JS_NewInt32(ctx, POMELO_SOCKET_CONNECT_DENIED)
    );

    JS_SetPropertyStr(
        ctx,
        connect_result,
        "TIMED_OUT",
        JS_NewInt32(ctx, POMELO_SOCKET_CONNECT_TIMED_OUT)
    );

    JS_SetModuleExport(ctx, m, "ConnectResult", connect_result);

    return 0;
}

