#ifndef POMELO_QUICKJS_TOKEN_SRC_H
#define POMELO_QUICKJS_TOKEN_SRC_H
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                                Public APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Initialize the token module
int pomelo_qjs_init_token_module(JSContext * ctx, JSModuleDef * m);


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

/// @brief Token.encode(...): Uint8Array
JSValue pomelo_qjs_token_encode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


/// @brief Token.randomBuffer(...): Uint8Array
JSValue pomelo_qjs_token_random_buffer(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_TOKEN_SRC_H
