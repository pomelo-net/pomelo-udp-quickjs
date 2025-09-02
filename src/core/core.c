#include "core.h"
#include "channel.h"
#include "context.h"
#include "message.h"
#include "session.h"
#include "socket.h"
#include "token.h"
#include "plugin.h"
#include "enums.h"
#include "functions.h"


/* -------------------------------------------------------------------------- */
/*                           Module initialization                            */
/* -------------------------------------------------------------------------- */



/// @brief Initialize the module
static int js_pomelo_init(JSContext * ctx, JSModuleDef * m) {
    // Initialize all modules
    if (pomelo_qjs_init_session_module(ctx, m) < 0) return -1;
    if (pomelo_qjs_init_channel_module(ctx, m) < 0) return -1;
    if (pomelo_qjs_init_message_module(ctx, m) < 0) return -1;
    if (pomelo_qjs_init_token_module(ctx, m) < 0) return -1;
    if (pomelo_qjs_init_plugin_module(ctx, m) < 0) return -1;
    if (pomelo_qjs_init_socket_module(ctx, m) < 0) return -1;

    // Initialize enums
    if (pomelo_qjs_init_enums(ctx, m) < 0) return -1;

    // Initialize global functions
    if (pomelo_qjs_init_global_functions(ctx, m) < 0) return -1;

    return 0;
}


JSModuleDef * js_init_module_pomelo(JSContext * ctx, const char * name) {
    JSModuleDef * m = JS_NewCModule(ctx, name, js_pomelo_init);
    if (!m) return NULL;

    JS_AddModuleExport(ctx, m, "Socket");
    JS_AddModuleExport(ctx, m, "Message");
    JS_AddModuleExport(ctx, m, "Token");
    JS_AddModuleExport(ctx, m, "Plugin");
    JS_AddModuleExport(ctx, m, "ChannelMode");
    JS_AddModuleExport(ctx, m, "ConnectResult");
    JS_AddModuleExport(ctx, m, "statistic");

    return m;
}
