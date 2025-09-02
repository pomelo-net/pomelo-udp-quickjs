#ifndef POMELO_QUICKJS_ENUMS_SRC_H
#define POMELO_QUICKJS_ENUMS_SRC_H
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Initialize all enums
int pomelo_qjs_init_enums(JSContext * ctx, JSModuleDef * m);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_ENUMS_SRC_H
