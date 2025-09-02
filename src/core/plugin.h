#ifndef POMELO_QUICKJS_PLUGIN_SRC_H
#define POMELO_QUICKJS_PLUGIN_SRC_H
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Initialize the token module
int pomelo_qjs_init_plugin_module(JSContext * ctx, JSModuleDef * m);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/


/// @brief Plugin.registerPluginByName(name: string)
JSValue pomelo_qjs_plugin_register_plugin_by_name(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Plugin.registerPluginByPath(path: string)
JSValue pomelo_qjs_plugin_register_plugin_by_path(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_PLUGIN_SRC_H
