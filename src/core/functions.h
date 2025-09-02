#ifndef POMELO_QUICKJS_FUNCTIONS_SRC_H
#define POMELO_QUICKJS_FUNCTIONS_SRC_H
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Initialize global functions
int pomelo_qjs_init_global_functions(JSContext * ctx, JSModuleDef * m);



/*----------------------------------------------------------------------------*/
/*                              Statistic APIs                                */
/*----------------------------------------------------------------------------*/

/// @brief statistic
JSValue pomelo_qjs_context_statistic(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_FUNCTIONS_SRC_H
