#ifndef HN_GAME_EXPRESSION_H
#define HN_GAME_EXPRESSION_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded native expression evaluation. Cursor side effects retain the GBA layout. */
bool hn_expression_evaluate(HNMemory *,GameAddr stream,bool update,uint32_t *result);
#ifdef __cplusplus
}
#endif
#endif
