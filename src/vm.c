#include <stdio.h>
// #if 1
// #define MOZVM_DUMP_OPCODE
// #endif
#include "instruction.h"
#include "mozvm_config.h"
#include "mozvm.h"
#include "pstring.h"
#include "ast.h"
#include "memo.h"
#include "token.h"
#include "symtable.h"

#ifdef __cplusplus
extern "C" {
#endif

moz_runtime_t *moz_runtime_init(unsigned jmptbl, unsigned memo)
{
    moz_runtime_t *r;
    unsigned size = sizeof(*r) + sizeof(long) * (MOZ_DEFAULT_STACK_SIZE - 1);
    r = (moz_runtime_t *)malloc(size);
    r->ast = AstMachine_init(MOZ_AST_MACHINE_DEFAULT_LOG_SIZE, NULL);
    r->table = symtable_init();
    r->memo = memo_init(MOZ_MEMO_DEFAULT_WINDOW_SIZE, memo, MEMO_TYPE_ELASTIC);
    r->head = r->input = r->tail = NULL;
    if (jmptbl) {
        r->jumps = (int *)malloc(sizeof(int) * MOZ_JMPTABLE_SIZE * jmptbl);
    }
    memset(r->stack_, 0xa, sizeof(long) * MOZ_DEFAULT_STACK_SIZE);
    r->stack = r->stack_ + 0xf;
    return r;
}

void moz_runtime_reset(moz_runtime_t *r)
{
}

void moz_runtime_dispose(moz_runtime_t *r)
{
    AstMachine_dispose(r->ast);
    symtable_dispose(r->table);
    memo_dispose(r->memo);
    if (r->jumps) {
        free(r->jumps);
    }

    free(r);
}

#define CONSUME() ++CURRENT;
#define CONSUME_N(N) CURRENT += N;

#if 0
#define FAIL() do {\
    long saved_, ast_tx_; \
    moz_inst_t *jump_; \
    char *pos_; \
    POP_FRAME(pos_, jump_, ast_tx_, saved_); \
    if (pos_ < CURRENT) { \
        HEAD = (HEAD < CURRENT) ? CURRENT : HEAD; \
        CURRENT = pos_; \
    } \
    ast_rollback_tx(AST_MACHINE_GET(), ast_tx_); \
    symtable_rollback(SYMTABLE_GET(), saved_); \
    PC = jump_; \
} while (0)
#else
#define FAIL() fprintf(stderr, "goto fail\n");goto L_fail;
#endif

#if 1
static long _POP(long **SP)
{
    long v;
    *SP -= 1;
    v = **SP;
    fprintf(stderr, "pop %ld\n", v);
    return v;
}
static void _PUSH(long **SP, long v)
{
    **SP = v;
    *SP += 1;
    fprintf(stderr, "push %ld\n", v);
}
#define PUSH(X)  _PUSH(&SP, (long)X)
#define POP()  _POP(&SP)
#else
#define PUSH(X) *SP++ = (long)(X)
#define POP()  *--SP
#endif
#define PEEK() PEEK_N(1)
#define PEEK_N(N) ((SP)+N)
#define ABORT() __asm volatile("int3")
#define TODO() __asm volatile("int3")
#define FP_FP     0
#define FP_POS    1
#define FP_NEXT   2
#define FP_AST    3
#define FP_SYMTBL 4
#define FP_MAX    (5)

#define PUSH_FRAME(POS, NEXT, AST, SYMTBL) do {\
    PUSH((long)FP); \
    PUSH(POS);\
    PUSH(NEXT);\
    PUSH(AST);\
    PUSH(SYMTBL);\
    FP = SP - FP_MAX;\
} while (0)

#define POP_FRAME(POS, NEXT, AST, SYMTBL) do {\
    SP     = FP; \
    SYMTBL = FP[FP_SYMTBL];\
    AST    = FP[FP_AST];\
    NEXT   = (moz_inst_t *)FP[FP_NEXT];\
    POS    = (char *)FP[FP_POS];\
    FP     = (long *)FP[FP_FP]; \
} while (0)

#define PEEK_FRAME(POS, NEXT, AST, SYMTBL) do {\
    SYMTBL = (FP+FP_SYMTBL);\
    AST    = (FP+FP_AST);\
    NEXT   = (moz_inst_t **)(FP+FP_NEXT);\
    POS    = (char **)(FP+FP_POS);\
} while (0)

#define DEBUG_CALL 1
int moz_runtime_parse(moz_runtime_t *runtime, char *CURRENT, char *end, moz_inst_t *PC)
{
#if DEBUG_CALL
    int call_stack = 0;
#endif
    long *SP = runtime->stack;
    long *FP = SP;
    long nterm_id = 0;
    AstMachine *AST = runtime->ast;
    symtable_t *TBL = runtime->table;
    memo_t *memo = runtime->memo;
    runtime->head = CURRENT;
    runtime->tail = end;
    AstMachine_setSource(AST, CURRENT);

    assert(*PC == Exit);
    fprintf(stderr, "%-8s SP=%p FP=%p %ld %s\n", runtime->nterms[nterm_id], SP, FP, (long)PC, "init");
    PUSH(0xaaaaaaaaL);
    PUSH(0xaaaaaaaaL);
    PUSH(nterm_id);
    PUSH(PC++);
    fprintf(stderr, "%-8s SP=%p FP=%p %ld %s\n", runtime->nterms[nterm_id], SP, FP, (long)PC, "init");
#define read_uint8_t(PC)  *(PC);             PC += sizeof(uint8_t)
#define read_int8_t(PC)   *((int8_t *)PC);   PC += sizeof(int8_t)
#define read_int(PC)      *((int *)PC);      PC += sizeof(int)
#define read_uint16_t(PC) *((uint16_t *)PC); PC += sizeof(uint16_t)
#define read_uint32_t(PC) *((uint32_t *)PC); PC += sizeof(uint32_t)
#define read_STRING_t(PC) *((STRING_t *)PC); PC += sizeof(STRING_t)
#define read_BITSET_t(PC) *((BITSET_t *)PC); PC += sizeof(BITSET_t)
#define read_TAG_t(PC)    *((TAG_t *)PC);    PC += sizeof(TAG_t)
#define read_JMPTBL_t(PC) *((JMPTBL_t *)PC); PC += sizeof(JMPTBL_t)

#define SYMTABLE_GET() (TBL)
#define AST_MACHINE_GET() (AST)
#define MEMO_GET() (memo)
#define HEAD (runtime)->head
#define EOS() (CURRENT == runtime->tail)
#define DISPATCH() goto L_vm_head
#define NEXT() DISPATCH()
#define JUMP(N) PC += N; DISPATCH()

#if 0
    uint8_t opcode = *PC;
#define DISPATCH_START(PC) L_vm_head:;switch ((opcode = *PC++)) {
#else
#define DISPATCH_START(PC) L_vm_head:;switch (*PC++) {
#endif
#define DISPATCH_END() default: ABORT(); }
#if 1
#define OP_CASE(OP) case OP:fprintf(stderr, "%-8s SP=%p FP=%p %ld %s\n", runtime->nterms[nterm_id], SP, FP, (long)(PC-1), #OP);
#else
#define OP_CASE(OP) case OP:
#endif
    DISPATCH_START(PC);
#include "vm_core.c"
    DISPATCH_END();
    assert(0 && "unreachable");
    return 0;
}

#ifdef __cplusplus
}
#endif
