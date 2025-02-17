.file "alder_lake_p.s"
.section .note.GNU-stack

.text

// p0 VRCPPS (XMM, XMM) - 1*p0 - Lat. 4
// p1 CRC32 - 1*p1 - Lat. 3
// p5 VEXTRACTF128 (XMM, YMM, I8) - 1*p5 - Lat. 3
// p06 ROR (R64, i8) - 1*p06 - Lat. 1


// For all: output_buffer in %rdi, num_entries in %rsi

.globl contend_p0_alder_lake_p
contend_p0_alder_lake_p:
    mfence
    rdtsc
    mfence
    .rept ((3 * 64) / 8) # (3 * 64) / latency
    vrcpps %xmm7, %xmm2
    vrcpps %xmm3, %xmm5
    vrcpps %xmm6, %xmm8
    vrcpps %xmm11, %xmm1
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p0_alder_lake_p
    vzeroall
    ret

.globl contend_p1_alder_lake_p
contend_p1_alder_lake_p:
    mfence
    rdtsc
    mfence
    .rept (3 * 64) / 6 # (3 * 64) / latency
    crc32 %r8, %r8
    crc32 %r9, %r9
    crc32 %r10, %r10
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p1_alder_lake_p
    vzeroall
    ret

.globl contend_p5_alder_lake_p
contend_p5_alder_lake_p:
    mfence
    rdtsc
    mfence
    .rept (3 * 64) / 6 # (3 * 64) / latency
    vextractf128 $1, %ymm0, %xmm1
    vextractf128 $1, %ymm2, %xmm3
    vextractf128 $1, %ymm4, %xmm5
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p5_alder_lake_p
    vzeroall
    ret

.globl contend_p06_alder_lake_p
contend_p06_alder_lake_p:
    mfence
    rdtsc
    mfence
    .rept (3 * 64) / 2 # (3 * 64) / latency
    ror $2, %rdx
    ror $2, %rcx
    #ror $2, %r9
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_p06_alder_lake_p
    vzeroall
    ret

