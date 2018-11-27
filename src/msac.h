/*
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DAV1D_SRC_MSAC_H__
#define __DAV1D_SRC_MSAC_H__

#include "config.h"

#include <stdint.h>
#include <stdlib.h>

/* Using uint32_t should be faster on 32 bit systems, in theory, maybe */
#define EC_WIN_SIZE (8)
typedef uint64_t ec_win;

typedef struct MsacContext {
    const uint8_t *buf_pos;
    const uint8_t *buf_end;
    ec_win dif;
    unsigned rng;
    int cnt;
    int allow_update_cdf;
} MsacContext;

void msac_init(MsacContext *c, const uint8_t *data, size_t sz, int disable_cdf_update_flag);
unsigned msac_decode_symbol_adapt(MsacContext *s, uint16_t *cdf,
                                  const unsigned n_symbols);
unsigned msac_decode_bools(MsacContext *c, unsigned l);
int msac_decode_subexp(MsacContext *c, int ref, int n, unsigned k);
int msac_decode_uniform(MsacContext *c, unsigned n);

unsigned msac_decode_bool_equi_c(MsacContext *const s);
unsigned msac_decode_bool_prob_c(MsacContext *s, unsigned f);
unsigned msac_decode_bool_adapt_c(MsacContext *s, uint16_t *cdf);
unsigned msac_decode_symbol_c(MsacContext *s, const uint16_t *cdf,
                              const unsigned n_symbols);

#if HAVE_ASM && ARCH_X86_64

unsigned dav1d_msac_decode_bool_equi(MsacContext *const s);
unsigned dav1d_msac_decode_bool_prob(MsacContext *const s, const unsigned f);
unsigned dav1d_msac_decode_bool_adapt(MsacContext *const s, uint16_t *c);
unsigned dav1d_msac_decode_symbol(MsacContext *s, const uint16_t *cdf,
                                  const unsigned n_symbols);

# define msac_decode_bool_equi dav1d_msac_decode_bool_equi
# define msac_decode_bool_prob dav1d_msac_decode_bool_prob
# define msac_decode_bool_adapt dav1d_msac_decode_bool_adapt
# define msac_decode_symbol dav1d_msac_decode_symbol
#else
# define msac_decode_bool_equi msac_decode_bool_equi_c
# define msac_decode_bool_prob msac_decode_bool_prob_c
# define msac_decode_bool_adapt msac_decode_bool_adapt_c
# define msac_decode_symbol msac_decode_symbol_c

#endif

#endif /* __DAV1D_SRC_MSAC_H__ */
