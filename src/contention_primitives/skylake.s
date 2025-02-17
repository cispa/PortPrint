.file "skylake.s"
.section .note.GNU-stack

.text

// For all: output_buffer in %rdi, num_entries in %rsi

.globl contend_p0_skylake
contend_p0_skylake:
    mfence
    rdtsc
    mfence
    .rept 48
    aesenc %xmm7, %xmm2
    aesenc %xmm3, %xmm5
    aesenc %xmm6, %xmm8
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p0_skylake
    ret

.globl contend_p1_skylake
contend_p1_skylake:
    mfence
    rdtsc
    mfence
    .rept 64
    crc32 %r8, %r8
    crc32 %r9, %r9
    crc32 %r10, %r10
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p1_skylake
    ret

.globl contend_p5_skylake
contend_p5_skylake:
    mfence
    rdtsc
    mfence
    .rept 64
    vpermd %ymm0, %ymm1, %ymm0
    vpermd %ymm2, %ymm3, %ymm2
    vpermd %ymm4, %ymm5, %ymm4
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p5_skylake
    ret

.globl contend_p06_skylake
contend_p06_skylake:
    mfence
    rdtsc
    mfence
    .rept 188
    ror $2, %rdx
    ror $2, %rcx
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p06_skylake
    ret

