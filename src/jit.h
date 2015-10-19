#include "mozvm.h"

#ifndef __JIT_H__
#define __JIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MOZVM_ENABLE_JIT
void mozvm_jit_init(moz_runtime_t *runtime);
void mozvm_jit_reset(moz_runtime_t *runtime);
void mozvm_jit_dispose(moz_runtime_t *runtime);
uint8_t mozvm_jit_call_prod(moz_runtime_t *runtime, const char *str, uint16_t prod_id);
moz_jit_func_t mozvm_jit_compile(moz_runtime_t *runtime, moz_production_t *e);

static inline int mozvm_prod_is_already_compiled(moz_production_t *e)
{
    return (e->compiled_code != mozvm_jit_call_prod);
}
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
#endif //__JIT_H__
