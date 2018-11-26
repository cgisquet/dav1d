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
  .upd: resd 1
endstruc

SECTION .text

%if UNIX64
%macro ctx_refill 0            ; unsigned ctx_refill(msac *s, unsigned ret) {
    mov rdx, [rdi + msac.pos]  ;   const uint8_t *pos = s->pos;
    mov rsi, [rdi + msac.end]  ;   const uint8_t *end = s->end;
    mov ecx, 40                ;   int c = EC_WIN_BITS - s->cnt - 24;
    sub ecx, [rdi + msac.cnt]
.loop:
    cmp  rdx, rsi              ;   while (buf_pos < buf_end) {
    jae .done
    movzx R9d, byte [rdx]      ;     dif ^= ((ec_win)*buf_pos++) << c;
    inc rdx
    shl  R9, cl
    xor R8, R9
    sub ecx, 8                 ;     c -= 8;
    jns .loop                  ;     if (c < 0) break;
.done:                         ;   }
    mov [rdi + msac.dif], R8   ;   s->dif = dif;
    mov [rdi + msac.pos], rdx  ;   s->pos = pos;
    mov edx, 40                ;   s->cnt = EC_WIN_BITS - c - 24;
    sub edx, ecx
    mov [rdi + msac.cnt], edx
    RET                        ;   return ret;
%endmacro                      ; }

%macro ctx_norm 0              ; unsigned ctx_norm(msac *s, ec_win dif,
                               ;                   unsigned rng, unsigned ret) {
    bsr edx, esi               ;   const uint16_t d = 15 - (31 ^ clz(rng));
    mov ecx, 15
    sub ecx, edx
    inc R8                     ;   s->dif = ((dif + 1) << d) - 1;
    shl R8, cl
    dec R8
    mov [rdi + msac.dif], R8
    shl esi, cl                ;   s->rng = rng << d;
    mov [rdi + msac.rng], esi
    sub [rdi + msac.cnt], ecx  ;   s->cnt -= d;
    js .refill                 ;   if (s->cnt >= 0) {
    RET                        ;     return ret;
.refill:                       ;   }
    ctx_refill                 ;   return ctx_refill(s, ret);
%endmacro                      ; }

%macro ctx_decode_bool 2       ; unsigned ctx_decode_bool(msac *s, unsigned p) {
    mov edx, [rdi + msac.rng]  ;   unsigned r = s->rng;
%if %1                         ; #if EQUI
    movzx esi, dh              ;   ec_win v = ((r >> 8) << 7) + EC_MIN_PROB;
    shl esi, 7
%else                          ; #else
    movzx eax, dh              ;   ec_win v = ((r >> 8) * p >> 1) + EC_MIN_PROB;
    imul  esi, eax
    shr esi, 1
%endif                         ; #endif
    add esi, 4
    mov R10d, esi              ;   ec_win vw = v << (EC_WIN_BITS - 16);
    shl R10, 48
    sub edx, esi               ;   unsigned new_v = r - v;
    xor eax, eax               ;   unsigned ret = 0;
    xor  R9, R9                ;   ec_win tmp = 0;
    mov  R8, [rdi + msac.dif]  ;   ec_win dif = s->dif;
    cmp  R8, R10               ;   if (dif >= vw) {
%if %2                         ; #if ADAPT
    cmovae R11d, eax           ;     adapt = 0;
%endif                         ; #endif
    cmovae esi, edx            ;     v = new_v
    cmovae R9, R10             ;     tmp = vw;
    setb al                    ;     ret = 1;
                               ;   }
    sub R8, R9                 ;   dif -= tmp;
%endmacro                      ; }

cglobal msac_decode_bool_equi, 1, 8, 0, ctx
                               ; unsigned msac_decode_bool_equi(msac *s) {
    ctx_decode_bool 1, 0       ;   unsigned ret = ctx_decode_bool(s, 256);
    ctx_norm                   ;   return ctx_norm(s, dif, v, ret);
                               ; }

cglobal msac_decode_bool_prob, 2, 8, 0, ctx, p
                               ; unsigned msac_decode_bool_prob(msac *s,
                               ;                                unsigned f) {
    ctx_decode_bool 0, 0       ;   unsigned ret = ctx_decode_bool(s, p);
    ctx_norm                   ;   return ctx_norm(s, dif, v, ret);
                               ; }

cglobal msac_decode_bool, 2, 8, 0, ctx, cdf
                               ; unsigned msac_decode_bool(msac *s,
                               ;                           uint16_t *cdf) {
    movzx esi, word [cdfq]     ;   unsigned p = cdf[0] >> EC_PROB_SHIFT;
    shr esi, 6
    ctx_decode_bool 0, 0       ;   unsigned ret = ctx_decode_bool(s, p);
    ctx_norm                   ;   return ctx_norm(s, dif, v, ret);
                               ; }

cglobal msac_decode_bool_adapt, 2, 10, 0, ctx, cdf
                               ; unsigned msac_decode_bool_adapt(msac *s,
                               ;                                uint16_t *cdf) {
    mov rbx, cdfq
    movzx ecx, byte [cdfq + 2] ;   int count = cdf[1];
    cmp ecx, 31                ;   cdf[1] += count < 32;
    setle al
    add byte [cdfq + 2], al
    shr ecx, 4                 ;   int rate = (count >> 4) | 4;
    or  ecx, 4
    mov R11d, 1                ;   int adapt = ((1 << rate) - 1) - 32768
    shl R11d, cl
    sub R11d, 0x8001
    movzx esi, word [cdfq]     ;   unsigned p = cdf[0] >> EC_PROB_SHIFT;;
    shr esi, 6
    ctx_decode_bool 0, 1       ;   unsigned ret = ctx_decode_bool(s, p);
    mov edx, [ctxq + msac.upd]
    test edx, edx
    jz .no_adapt               ;   if (s->upd) {
    movsx edx, word [rbx]      ;     cdf[1] -= (cdf[1] + adapt) >> rate;
    add   edx, R11d
    sar   edx, cl
    sub word [rbx], dx
.no_adapt:                     ;   }
    ctx_norm                   ;   return ctx_norm(s, dif, v, ret);
                               ; }
%endif
