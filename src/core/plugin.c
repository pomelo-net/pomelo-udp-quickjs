#include <assert.h>
#include "pomelo/api.h"
#include "context.h"
#include "plugin.h"


int pomelo_qjs_init_plugin_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    JSValue plugin = JS_NewObject(ctx);

    // Create static functions `registerPluginByName`
    JSValue fn_reg_by_name = JS_NewCFunction(
        ctx,
        pomelo_qjs_plugin_register_plugin_by_name,
        "registerPluginByName",
        /* argc = */ 1
    );
    JS_SetPropertyStr(ctx, plugin, "registerPluginByName", fn_reg_by_name);

    // Create static functions `registerPluginByPath`
    JSValue fn_reg_by_path = JS_NewCFunction(
        ctx,
        pomelo_qjs_plugin_register_plugin_by_path,
        "registerPluginByPath",
        /* argc = */ 1
    );
    JS_SetPropertyStr(ctx, plugin, "registerPluginByPath", fn_reg_by_path);

    JS_SetModuleExport(ctx, m, "Plugin", plugin);
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


JSValue pomelo_qjs_plugin_register_plugin_by_name(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) thiz;
    if (argc < 1) {
        return JS_ThrowInternalError(ctx, "Missing required arguments");
    }

    JSValue name = argv[0];
    if (!JS_IsString(name)) {
        return JS_ThrowInternalError(ctx, "Invalid arguments");
    }

    const char * name_str = JS_ToCString(ctx, name);
    if (!name_str) {
        return JS_ThrowInternalError(ctx, "Failed to convert string");
    }

    pomelo_plugin_initializer initializer =
        pomelo_plugin_load_by_name(name_str);
    if (!initializer) {
        JS_FreeCString(ctx, name_str);
        return JS_NewBool(ctx, false);
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_plugin_t * plugin = pomelo_plugin_register(
        context->allocator,
        context->context,
        context->platform,
        initializer
    );

    JS_FreeCString(ctx, name_str);
    return JS_NewBool(ctx, plugin != NULL);
}


JSValue pomelo_qjs_plugin_register_plugin_by_path(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) thiz;
    if (argc < 1) {
        return JS_ThrowInternalError(ctx, "Missing required arguments");
    }

    JSValue path = argv[0];
    if (!JS_IsString(path)) {
        return JS_ThrowInternalError(ctx, "Invalid arguments");
    }

    const char * path_str = JS_ToCString(ctx, path);
    if (!path_str) {
        return JS_ThrowInternalError(ctx, "Failed to convert string");
    }

    pomelo_plugin_initializer initializer =
        pomelo_plugin_load_by_path(path_str);
    if (!initializer) {
        JS_FreeCString(ctx, path_str);
        return JS_NewBool(ctx, false);
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    pomelo_plugin_t * plugin = pomelo_plugin_register(
        context->allocator,
        context->context,
        context->platform,
        initializer
    );

    JS_FreeCString(ctx, path_str);
    return JS_NewBool(ctx, plugin != NULL);
}
