// MIT: independently written from Apple ARM64 ABI / AAPCS64, not a copied coroutine library.
// Normal arm64 only (not arm64e / authenticated return addresses). x18 is reserved by Apple.
// A normal external call boundary preserves x19-x29, SP and the low 64 bits of v8-v15.
// Save LR as the continuation and FPCR/FPSR as well: the game uses non-default FP modes.
#if !defined(__aarch64__) || defined(__arm64e__)
#error Unsupported context-switch ABI
#endif
.text
.p2align 2
.globl _rac1_arm64_context_swap
_rac1_arm64_context_swap:
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x9, sp
    str x9, [x0, #96]
    stp d8, d9, [x0, #104]
    stp d10, d11, [x0, #120]
    stp d12, d13, [x0, #136]
    stp d14, d15, [x0, #152]
    mrs x9, fpcr
    str x9, [x0, #168]
    mrs x9, fpsr
    str x9, [x0, #176]
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldp x29, x30, [x1, #80]
    ldr x9, [x1, #96]
    mov sp, x9
    ldp d8, d9, [x1, #104]
    ldp d10, d11, [x1, #120]
    ldp d12, d13, [x1, #136]
    ldp d14, d15, [x1, #152]
    ldr x9, [x1, #168]
    msr fpcr, x9
    ldr x9, [x1, #176]
    msr fpsr, x9
    isb
    ret
