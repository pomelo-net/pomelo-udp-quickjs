#ifndef POMELO_QUICKJS_CORE_SRC_H
#define POMELO_QUICKJS_CORE_SRC_H
#include "pomelo-qjs/core.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The binding qjs message
typedef struct pomelo_qjs_message_s pomelo_qjs_message_t;

/// @brief The binding qjs session
typedef struct pomelo_qjs_session_s pomelo_qjs_session_t;

/// @brief The binding qjs socket
typedef struct pomelo_qjs_socket_s pomelo_qjs_socket_t;

/// @brief The binding qjs channel
typedef struct pomelo_qjs_channel_s pomelo_qjs_channel_t;

/// @brief The send info
typedef struct pomelo_qjs_send_info_s pomelo_qjs_send_info_t;

/// @brief The standard functions
typedef struct pomelo_qjs_std_s pomelo_qjs_std_t;


/// @brief [External linkage] platform statistic
JSValue pomelo_qjs_platform_statistic(
    JSContext * ctx, pomelo_platform_t * platform
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_CORE_SRC_H
