#ifndef POMELO_QJS_CONSOLE_SRC_H
#define POMELO_QJS_CONSOLE_SRC_H
#include "std.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The maximum number of arguments for logging
#define POMELO_QJS_CONSOLE_MAX_LOG_ARGS 256


/// @brief Initialize global console object
int pomelo_qjs_console_init(pomelo_qjs_context_t * context);


/// @brief Cleanup global console object
void pomelo_qjs_console_cleanup(pomelo_qjs_context_t * context);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_CONSOLE_SRC_H
