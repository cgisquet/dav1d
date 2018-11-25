; Copyright © 2018, VideoLAN and dav1d authors
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; 1. Redistributions of source code must retain the above copyright notice, this
;    list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
; ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%include "config.asm"
%include "ext/x86/x86inc.asm"

struc msac
  .pos: resq 1
  .end: resq 1
  .dif: resq 1
  .rng: resd 1
  .cnt: resd 1
endstruc

SECTION .text

%if UNIX64
%macro msac_refill 1           ; unsigned ctx_refill(msac *s, unsigned ret) {
    mov  r1, [%1 + msac.dif]   ;   ec_win dif = s->dif;
    mov  r2, [%1 + msac.pos]   ;   const uint8_t *pos = s->pos;
    mov  r4, [%1 + msac.end]   ;   const uint8_t *end = s->end;
    mov r3d, 40                ;   int c = EC_WIN_BITS - s->cnt - 24;
    sub r3d, [%1 + msac.cnt]
    js .skip                   ;   if (c >= 0) {
.loop:
    cmp  r2, r4                ;     while (buf_pos < buf_end) {
    jae .done
    movzx r5d, byte [r2]       ;       dif ^= ((ec_win)*buf_pos++) << c;
    inc  r2
    shl  r5, r3b
    xor  r1, r5
    sub r3d, 8                 ;       c -= 8;
    jns .loop                  ;       if (c < 0) break;
.done:                         ;     }
.skip:                         ;   }
    mov [%1 + msac.dif], r1    ;   s->dif = dif;
    mov [%1 + msac.pos], r2    ;   s->pos = pos;
    mov r2d, 40                ;   s->cnt = EC_WIN_BITS - c - 24;
    sub r2d, r3d
    mov [%1 + msac.cnt], r2d
    RET                        ;   return ret;
%endmacro                      ; }

%macro msac_norm 3             ; unsigned ctx_norm(msac *s, ec_win dif,
                               ;                   unsigned rng, unsigned ret) {
    bsr r2d, %3                ;   const uint16_t d = 15 - (31 ^ clz(rng));
    mov r3d, 15
    sub r3d, r2d
    inc %2                     ;   s->dif = ((dif + 1) << d) - 1;
    shl %2, r3b
    dec %2
    mov [%1 + msac.dif], %2
    shl %3, r3b                ;   s->rng = rng << d;
    mov [%1 + msac.rng], %3
    sub [%1 + msac.cnt], r3d   ;   s->cnt -= d;
    js .refill                 ;   if (s->cnt >= 0) {
    RET                        ;     return ret;
.refill:                       ;   }
    msac_refill %1             ;   return msac_refill(s, ret);
%endmacro                      ; }

cglobal msac_decode_bool_equi, 1, 6, 0, ctx
                               ; unsigned msac_decode_bool_equi(msac *s) {
    mov  r1, [ctxq + msac.dif] ;   ec_win dif = s->dif;
    mov r2d, [ctxq + msac.rng] ;   unsigned r = s->rng;
    movzx r3d, r2h             ;   ec_win v = ((r >> 8) << 7) + EC_MIN_PROB;
    shl r3d, 7
    add r3d, 4
    mov r4d, r3d
    shl  r3, 48                ;   ec_win vw = v << (EC_WIN_BITS - 16);
    xor  r5, r5                ;   ec_win tmp = 0;
    xor r6d, r6d               ;   unsigned ret = 0;
    sub r2d, r4d               ;   unsigned new_v = r - v;
    cmp  r1, r3                ;   if (dif >= vw) {
    setb r6b                   ;     ret = 1;
    cmovae r4d, r2d            ;     v = new_v;
    cmovae r5, r3              ;     tmp = vw;
                               ;   }
    sub  r1, r5                ;   dif -= tmp;
    msac_norm ctxq, r1, r4d    ;   return msac_norm(s, dif, v, ret);
                               ; }

cglobal msac_decode_bool_prob, 2, 6, 0, ctx, prob
                               ; unsigned msac_decode_bool_prob(msac *s,
                               ;                                unsigned f) {
    mov  r4, [ctxq + msac.dif] ;   ec_win dif = s->dif;
    mov r2d, [ctxq + msac.rng] ;   unsigned r = s->rng;
    movzx r3d, r2h             ;   ec_win v = ((r >> 8) * f >> 1) + EC_MIN_PROB;
    imul r1d, r3d
    shr r1d, 1
    add r1d, 4
    mov r3d, r1d
    shl r3, 48                 ;   ec_win vw = v << (EC_WIN_BITS - 16);
    sub r2d, r1d               ;   unsigned new_v = r - v;
    xor r5, r5                 ;   ec_win tmp = 0;
    xor r6d, r6d               ;   unsigned ret = 0;
    cmp r4, r3                 ;   if (dif >= vw) {
    setb r6b                   ;     ret = 1;
    cmovae r1d, r2d            ;     v = new_v
    cmovae r5, r3              ;     tmp = vw;
                               ;   }
    sub  r4, r5                ;   dif -= tmp;
    msac_norm ctxq, r4, r1d    ;   return msac_norm(s, dif, v, ret);
                               ; }
%endif
