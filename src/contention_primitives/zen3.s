.file "zen3.s"
.section .note.GNU-stack

.text

// DIV: IDIV (r8l) - lat 10 
// ALU 1: IMUL (r32, r32) - lat 3
// ALU 0/3: CMOVZ (r64, r64) - lat 1
// ALU 1/2: ROR (r64, i8) - lat 1

// FP1: VPINSRQ (XMM, XMM; r64, i8) - lat [1;6]
// FP3: VCVTSI2SS (XMM, XMM, R64) - lat [0;9]
// FP01: VBROADCASTSD (YMM, XMM) - lat 1
// FP12: VPSHUFLW: (XMM, XMM, i8) - lat 1
// FP45: VMOVMSKPS (R32, YMM) - lat <7

// For all: output_buffer in %rdi, num_entries in %rsi

.globl contend_div_zen3
contend_div_zen3:
    mfence
    rdtsc
    mfence
    mov %eax, %r10d
    mov $0x10, %rax
    mov $1, %cl
    mov $1, %dl
    mov $1, %r8b
    .rept ((2 * 64) / 20) # LAT 10
    idiv %cl
    idiv %dl
    idiv %r8b
    .endr
    shl $0x20, %r11
    or %r11, %r10
    mov %r10d, %r11d
    dec %rsi
    mov %r10, (%rdi, %rsi, 8)
    jnz contend_div_zen3
    vzeroall
    ret

.globl contend_alu1_zen3
contend_alu1_zen3:
    mfence
    rdtsc
    mfence
    .rept ((2 * 64) / 3) # LAT 3
    imul %rcx, %rdx
    imul %rsi, %rcx
    imul %r8, %r9
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_alu1_zen3
    vzeroall
    ret

.globl contend_alu03_zen3
contend_alu03_zen3:
    mfence
    rdtsc
    mfence
    .rept ((2 * 64) / 1) # LAT 1
    cmovz %rcx, %rdx
    cmovz %rsi, %rcx
    cmovz %r8, %r9
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_alu03_zen3
    vzeroall
    ret

.globl contend_alu12_zen3
contend_alu12_zen3:
    mfence
    rdtsc
    mfence
    .rept ((2 * 64) / 1) # LAT 1
    ror $2, %rdx
    ror $7, %rcx
    ror $11, %r9
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_alu12_zen3
    vzeroall
    ret

.globl contend_fp1_zen3
contend_fp1_zen3:
    mfence
    rdtsc
    mfence
    .rept ((4 * 64) / 4) # LAT [1;6]
    vpinsrq $1, %rdx, %xmm2, %xmm5
    vpinsrq $2, %rcx, %xmm6, %xmm3
    vpinsrq $9, %r10, %xmm7, %xmm15
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_fp1_zen3
    vzeroall
    ret

.globl contend_fp3_zen3
contend_fp3_zen3:
    mfence
    rdtsc
    mfence
    .rept ((3 * 64) / 3) # TOOD LAT [0;9]
    vcvtsi2ss %rdx, %xmm2, %xmm5
    vcvtsi2ss %rcx, %xmm6, %xmm3
    vcvtsi2ss %r10, %xmm7, %xmm15
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_fp3_zen3
    vzeroall
    ret

.globl contend_fp01_zen3
contend_fp01_zen3:
    mfence
    rdtsc
    mfence
    .rept ((1 * 64) / 1) # TOOD LAT 1
    vbroadcastsd %xmm2, %ymm5
    vbroadcastsd %xmm6, %ymm3
    vbroadcastsd %xmm7, %ymm15
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_fp01_zen3
    vzeroall
    ret

.globl contend_fp12_zen3
contend_fp12_zen3:
    mfence
    rdtsc
    mfence
    .rept ((2 * 64) / 1) # TOOD LAT 1
    vpshuflw $0x33, %xmm2, %xmm5
    vpshuflw $0x4e, %xmm6, %xmm3
    vpshuflw $0xe4, %xmm7, %xmm15
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_fp12_zen3
    vzeroall
    ret

.globl contend_fp45_zen3
contend_fp45_zen3:
    mfence
    rdtsc
    mfence
    .rept ((2 * 64) / 2) # TOOD LAT <=7
    vmovmskps %ymm5, %edx
    vmovmskps %ymm3, %r9d
    vmovmskps %ymm15, %ecx
    .endr
    shl $0x20, %r11
    or %r11, %rax
    mov %eax, %r11d
    dec %rsi
    mov %rax, (%rdi, %rsi, 8)
    jnz contend_fp45_zen3
    vzeroall
    ret
