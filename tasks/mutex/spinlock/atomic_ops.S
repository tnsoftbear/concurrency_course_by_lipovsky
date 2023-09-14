#if(APPLE)
  #define FUNCTION_NAME(name) _##name
#else
  #define FUNCTION_NAME(name) name
#endif

.globl FUNCTION_NAME(AtomicLoad)
.globl FUNCTION_NAME(AtomicStore)
.globl FUNCTION_NAME(AtomicExchange)

# Solution starts here

FUNCTION_NAME(AtomicLoad):
    movq (%rdi), %rax
    retq

FUNCTION_NAME(AtomicStore):
    xchg %rsi, (%rdi)
    retq

FUNCTION_NAME(AtomicExchange):
    movq %rsi, %rax
    xchg %rax, (%rdi)
    retq
 