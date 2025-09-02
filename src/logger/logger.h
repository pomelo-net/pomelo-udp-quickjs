#ifndef POMELO_QJS_LOGGER_SRC_H
#define POMELO_QJS_LOGGER_SRC_H
#include "pomelo-qjs/logger.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Initialize the logger
int pomelo_qjs_logger_init(pomelo_qjs_context_t * context);


/// @brief Cleanup the logger
void pomelo_qjs_logger_cleanup(pomelo_qjs_context_t * context);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_LOGGER_SRC_H
