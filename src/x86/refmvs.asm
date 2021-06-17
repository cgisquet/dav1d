%include "config.asm"
%include "ext/x86/x86inc.asm"

SECTION_RODATA 16

%macro JMP_TABLE 2-*
    %xdefine %1_table (%%table - 2*%2)
    %xdefine %%base %1_table
    %xdefine %%prefix mangle(private_prefix %+ _%1)
    %%table:
    %rep %0 - 1
        dd %%prefix %+ .loop%2 - %%base
        %rotate 1
    %endrep
%endmacro

JMP_TABLE splat_mv_sse2, 4, 8, 16, 32

SECTION .text

INIT_XMM sse2
; refmvs_block **rr, int bx4, int bw4, int bh4, refmvs_block *a
cglobal splat_mv, 5, 5, 3, rr, bx4, bw4, bh4, a
    movzx         bx4q, bx4w
    imul          bx4q, 12
    cmp           bw4q, 2
    jle        .start2

.dummy:
    tzcnt         bw4d, bw4m
    mova            m0, [aq]
    LEA             aq, splat_mv_sse2_table
    movsxd        bw4q, dword [aq+bw4q*4]
    add           bw4q, aq
    mov             aq, [rrq]
    add             aq, bx4q
    pshufd      m2, m0, q2102
    pshufd      m1, m0, q1021
    pshufd      m0, m0, q0210
    jmp           bw4q

.loop32:
    mova     [aq+ 368], m2
    mova     [aq+ 352], m1
    mova     [aq+ 336], m0
    mova     [aq+ 320], m2
    mova     [aq+ 304], m1
    mova     [aq+ 288], m0
    mova     [aq+ 272], m2
    mova     [aq+ 256], m1
    mova     [aq+ 240], m0
    mova     [aq+ 224], m2
    mova     [aq+ 208], m1
    mova     [aq+ 192], m0
.loop16:
    mova     [aq+ 176], m2
    mova     [aq+ 160], m1
    mova     [aq+ 144], m0
    mova     [aq+ 128], m2
    mova     [aq+ 112], m1
    mova     [aq+  96], m0
.loop8:
    mova     [aq+  80], m2
    mova     [aq+  64], m1
    mova     [aq+  48], m0
.loop4:
    mova     [aq+  32], m2
    mova     [aq+  16], m1
    mova     [aq+   0], m0
    dec           bh4q
    jz            .end
    add            rrq, gprsize
    mov             aq, [rrq]
    add             aq, bx4q
    jmp           bw4q
.end:
    RET

.start2:
    cmp           bw4q, 1
    jle        .start1
    mova            m0, [aq]
    pshufd      m1, m0, q0021
    pshufd      m0, m0, q0210
    mov             aq, [rrq]
.loop2:
    movu  [aq+bx4q+ 0], m0
    movh  [aq+bx4q+16], m1
    dec           bh4q
    jz            .end
    add            rrq, gprsize
    mov             aq, [rrq]
    jmp         .loop2

.start1:
    movh            m0, [aq]
    mov           bw4d, dword [aq+8]
    mov             aq, [rrq]
.loop1:
    movh   [aq+bx4q+0], m0
    mov    [aq+bx4q+8], bw4d
    dec           bh4q
    jz            .end
    add            rrq, gprsize
    mov             aq, [rrq]
    jmp         .loop1
