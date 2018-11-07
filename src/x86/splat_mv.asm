%include "config.asm"
%include "ext/x86/x86inc.asm"

SECTION .text

INIT_XMM sse
; refmvs *r, ptrdiff_t stride, int bw4, int bh4, aliasmv *a
cglobal splat_mv, 5, 5, 3, r, stride, bw4, bh4, a
    movzx         bw4q, bw4w
    movzx         bh4q, bh4w
    imul       strideq, 12
    cmp           bw4d, 1
    jle        .start1
    cmp           bw4d, 2
    jle        .start2

    mova            m0, [aq+0]
    mova            m1, m0
    mova            m2, m0
    shufps      m0, m0, q0210
    shufps      m1, m1, q1021
    shufps      m2, m2, q2102
    cmp           bw4d, 32
    jge        .loop32
    cmp           bw4d, 4
    jle         .loop4

    shr           bh4q, 1
    cmp           bw4d, 16
    jge        .loop16

.loop8: ; 2 rows at a time
    mova     [rq+  80], m2
    mova     [rq+  64], m1
    mova     [rq+  48], m0
;.loop4:
    mova     [rq+  32], m2
    mova     [rq+  16], m1
    mova     [rq+   0], m0

    mova     [rq+ strideq+ 80], m2
    mova     [rq+ strideq+ 64], m1
    mova     [rq+ strideq+ 48], m0
;.loop4:
    mova     [rq+ strideq+ 32], m2
    mova     [rq+ strideq+ 16], m1
    mova     [rq+ strideq+  0], m0
    lea             rq, [rq+2*strideq]
    dec           bh4q
    jnz         .loop8
    RET

.loop32:
    mova     [rq+ 368], m2
    mova     [rq+ 352], m1
    mova     [rq+ 336], m0
    mova     [rq+ 320], m2
    mova     [rq+ 304], m1
    mova     [rq+ 288], m0
    mova     [rq+ 272], m2
    mova     [rq+ 256], m1
    mova     [rq+ 240], m0
    mova     [rq+ 224], m2
    mova     [rq+ 208], m1
    mova     [rq+ 192], m0
;.loop16:
    mova     [rq+ 176], m2
    mova     [rq+ 160], m1
    mova     [rq+ 144], m0
    mova     [rq+ 128], m2
    mova     [rq+ 112], m1
    mova     [rq+  96], m0
;.loop8:
    mova     [rq+  80], m2
    mova     [rq+  64], m1
    mova     [rq+  48], m0
;.loop4:
    mova     [rq+  32], m2
    mova     [rq+  16], m1
    mova     [rq+   0], m0
    add             rq, strideq
    dec           bh4q
    jge        .loop32
    RET

.loop16: ; 2 rows at a time
    mova     [rq+ 176], m2
    mova     [rq+ 160], m1
    mova     [rq+ 144], m0
    mova     [rq+ 128], m2
    mova     [rq+ 112], m1
    mova     [rq+  96], m0
;.loop8:
    mova     [rq+  80], m2
    mova     [rq+  64], m1
    mova     [rq+  48], m0
;.loop4:
    mova     [rq+  32], m2
    mova     [rq+  16], m1
    mova     [rq+   0], m0
    add             rq, strideq

    mova     [rq+ 176], m2
    mova     [rq+ 160], m1
    mova     [rq+ 144], m0
    mova     [rq+ 128], m2
    mova     [rq+ 112], m1
    mova     [rq+  96], m0
;.loop8:
    mova     [rq+  80], m2
    mova     [rq+  64], m1
    mova     [rq+  48], m0
;.loop4:
    mova     [rq+  32], m2
    mova     [rq+  16], m1
    mova     [rq+   0], m0
    add             rq, strideq
    dec           bh4q
    jnz        .loop16
    RET

.loop4:
    mova     [rq+  32], m2
    mova     [rq+  16], m1
    mova     [rq+   0], m0
    add             rq, strideq
    dec           bh4q
    jnz         .loop4
    RET

.start1:
    movlps          m0, [aq]
    mov           bw4d, dword [aq+8]
.loop1:
    movlps    [rq+  0], m0
    mov dword [rq+  8], bw4d
    add             rq, strideq
    dec           bh4q
    jnz         .loop1
    RET

.start2:
    mova            m0, [aq+0]
    mova            m0, [aq+0]
    mova            m1, m0
    shufps      m0, m0, q0210
    ;psrldq          m1, 32
    shufps      m1, m1, q0021
.loop2:
    movu      [rq+  0], m0
    movlps    [rq+ 16], m1
    add             rq, strideq
    dec           bh4q
    jnz         .loop2
    RET
