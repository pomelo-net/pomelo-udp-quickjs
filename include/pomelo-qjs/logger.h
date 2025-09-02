#ifndef POMELO_QJS_LOGGER_H
#define POMELO_QJS_LOGGER_H
#include "pomelo-qjs/core.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    POMELO_QJS_LOGGER_LEVEL_DEBUG,
    POMELO_QJS_LOGGER_LEVEL_LOG,
    POMELO_QJS_LOGGER_LEVEL_INFO,
    POMELO_QJS_LOGGER_LEVEL_WARN,
    POMELO_QJS_LOGGER_LEVEL_ERROR
} pomelo_qjs_logger_level;


/// @brief Log a message
void pomelo_qjs_logger_log(
    pomelo_qjs_context_t * context,
    pomelo_qjs_logger_level level,
    const char * fmt,
    ...
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QJS_LOGGER_H
