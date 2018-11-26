/*
 * Copyright © 2018, VideoLAN and dav1d authors
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

#include "src/cpu.h"
#include "tests/checkasm/checkasm.h"
#include "src/msac.h"
#include <stdio.h>
#include <inttypes.h>

#define decl_msac_decode_bool_equi_fn(name) \
unsigned (name)(MsacContext *const s)
typedef decl_msac_decode_bool_equi_fn(*msac_decode_bool_equi_fn);

#define decl_msac_decode_bool_prob_fn(name) \
unsigned (name)(MsacContext *const s, unsigned f)
typedef decl_msac_decode_bool_prob_fn(*msac_decode_bool_prob_fn);

#define decl_msac_decode_bool_adapt_fn(name) \
unsigned (name)(MsacContext *const s, uint16_t *c)
typedef decl_msac_decode_bool_adapt_fn(*msac_decode_bool_adapt_fn);

unsigned dav1d_msac_decode_bool_equi(MsacContext *const s);
unsigned dav1d_msac_decode_bool_prob(MsacContext *const s, const unsigned f);
unsigned dav1d_msac_decode_bool_adapt(MsacContext *const s, uint16_t *c);

#define MAX_BYTES (100000)
#define MAX_BITS  MAX_BYTES

static unsigned char buf[MAX_BYTES];
static unsigned prob[MAX_BITS];

static int msac_equal(MsacContext *a, MsacContext *b) {
    return a->buf_pos == b->buf_pos && a->buf_end == b->buf_end &&
        a->dif == b->dif && a->rng == b->rng && a->cnt == b->cnt;
}

static void print_msac(MsacContext *s, const char *name) {
    printf("%s: pos = %p, end = %p, dif = %" PRIx64 ", rng = %x, cnt = %i\n",
        name, s->buf_pos, s->buf_end, s->dif, s->rng, s->cnt);
}

static void check_decode_bool_equi() {
    int i;

    declare_func(unsigned, MsacContext *const s);

    msac_decode_bool_equi_fn decode_bool_equi;

    decode_bool_equi = msac_decode_bool_equi_c;
#if !defined(_WIN64) && ARCH_X86_64
    if (dav1d_get_cpu_flags() & DAV1D_X86_CPU_FLAG_AVX2) {
        decode_bool_equi = dav1d_msac_decode_bool_equi;
    }
#endif

    if (check_func(decode_bool_equi, "msac_decode_bool_equi")) {
        MsacContext s_ref;
        MsacContext s_new;

        msac_init(&s_ref, buf, MAX_BYTES, 0);
        msac_init(&s_new, buf, MAX_BYTES, 0);

        /* Decode MAX_BYTES worth of bits */
        for (i = 0; i < MAX_BYTES; i++) {
            unsigned ref;
            unsigned new;
            ref = call_ref(&s_ref);
            new = call_new(&s_new);
            if (ref != new || !msac_equal(&s_ref, &s_new)) {
                fail();
            }
        }

        msac_init(&s_new, buf, MAX_BYTES, 0);
        bench_new(&s_new);
    }
    report("decode_bool_equi");
}

static void check_decode_bool_prob() {
    int i;

    declare_func(unsigned, MsacContext *const s, const unsigned f);

    msac_decode_bool_prob_fn decode_bool_prob;

    decode_bool_prob = msac_decode_bool_prob_c;
#if !defined(_WIN64) && ARCH_X86_64
    if (dav1d_get_cpu_flags() & DAV1D_X86_CPU_FLAG_AVX2) {
        decode_bool_prob = dav1d_msac_decode_bool_prob;
    }
#endif

    if (check_func(decode_bool_prob, "msac_decode_bool_prob")) {
        MsacContext s_ref;
        MsacContext s_new;

        msac_init(&s_ref, buf, MAX_BYTES, 0);
        msac_init(&s_new, buf, MAX_BYTES, 0);

        /* Decode MAX_BYTES worth of bits */
        for (i = 0; i < MAX_BITS; i++) {
            unsigned ref;
            unsigned new;
            ref = call_ref(&s_ref, prob[i]);
            new = call_new(&s_new, prob[i]);
            if (ref != new) {
                fail();
            }
        }

        msac_init(&s_new, buf, MAX_BYTES, 0);
        bench_new(&s_new, prob[0]);
    }
    report("decode_bool_prob");
}

static int cdf2_equal(const uint16_t *const a, const uint16_t *const b) {
  return a[0] == b[0] && a[1] == b[1];
}

static void print_cdf2(const uint16_t *const c, const char *name) {
    printf("%s: cdf[0] = %i, cdf[1] = %i\n", name, c[0], c[1]);
}

static void check_decode_bool_no_adapt() {
    int i;

    declare_func(unsigned, MsacContext *const s, uint16_t *cdf);

    msac_decode_bool_adapt_fn decode_bool_adapt;

    decode_bool_adapt = msac_decode_bool_adapt_c;
#if !defined(_WIN64) && ARCH_X86_64
    if (dav1d_get_cpu_flags() & DAV1D_X86_CPU_FLAG_AVX2) {
        decode_bool_adapt = dav1d_msac_decode_bool_adapt;
    }
#endif
    if (check_func(decode_bool_adapt, "msac_decode_bool_no_adapt")) {
        MsacContext s_ref;
        MsacContext s_new;

        msac_init(&s_ref, buf, MAX_BYTES, 1);
        msac_init(&s_new, buf, MAX_BYTES, 1);

        uint16_t c_ref[2] = { 16384, 0 };
        uint16_t c_new[2] = { 16384, 0 };

        /* Decode MAX_BYTES worth of bits */
        for (i = 0; i < MAX_BITS; i++) {
            unsigned ref;
            unsigned new;
            ref = call_ref(&s_ref, c_ref);
            new = call_new(&s_new, c_new);
            if (ref != new || c_ref[0] != c_new[0] ||
                !msac_equal(&s_ref, &s_new)) {
                printf("i = %i, ref = %i, new = %i\n", i, ref, new);
                print_cdf2(c_ref, "ref");
                print_cdf2(c_new, "new");
                print_msac(&s_ref, "ref");
                print_msac(&s_new, "new");
                fail();
                break;
            }
        }

        msac_init(&s_new, buf, MAX_BYTES, 1);
        c_new[0] = 16384;
        c_new[1] = 0;
        bench_new(&s_new, c_new);
    }
    report("decode_bool_no_adapt");
}

static void check_decode_bool_adapt() {
    int i;

    declare_func(unsigned, MsacContext *const s, uint16_t *cdf);

    msac_decode_bool_adapt_fn decode_bool_adapt;

    decode_bool_adapt = msac_decode_bool_adapt_c;
#if !defined(_WIN64) && ARCH_X86_64
    if (dav1d_get_cpu_flags() & DAV1D_X86_CPU_FLAG_AVX2) {
        decode_bool_adapt = dav1d_msac_decode_bool_adapt;
    }
#endif
    if (check_func(decode_bool_adapt, "msac_decode_bool_adapt")) {
        MsacContext s_ref;
        MsacContext s_new;

        msac_init(&s_ref, buf, MAX_BYTES, 0);
        msac_init(&s_new, buf, MAX_BYTES, 0);

        uint16_t c_ref[2] = { 16384, 0 };
        uint16_t c_new[2] = { 16384, 0 };

        /* Decode MAX_BYTES worth of bits */
        for (i = 0; i < MAX_BITS; i++) {
            unsigned ref;
            unsigned new;
            ref = call_ref(&s_ref, c_ref);
            new = call_new(&s_new, c_new);
            if (ref != new || !cdf2_equal(c_ref, c_new) ||
                !msac_equal(&s_ref, &s_new)) {
                printf("i = %i, g ref = %i, new = %i\n", i, ref, new);
                print_cdf2(c_ref, "ref");
                print_cdf2(c_new, "new");
                print_msac(&s_ref, "ref");
                print_msac(&s_new, "new");
                fail();
                break;
            }
        }

        msac_init(&s_new, buf, MAX_BYTES, 0);
        c_new[0] = 16384;
        c_new[1] = 0;
        bench_new(&s_new, c_new);
    }
    report("decode_bool_adapt");
}

static void check_init() {
    int i;

    /* Create a random EC segment */
    for (i = 0; i < MAX_BYTES; i++) {
        buf[i] = rand() & 0xff;
    }

    /* Create a random set of probabilities */
    for (i = 0; i < MAX_BITS; i++) {
        prob[i] = (rand() % 0x1ff) + 1;
    }
}

void checkasm_check_msac(void) {
    check_init();
    check_decode_bool_equi();
    check_decode_bool_prob();
    check_decode_bool_no_adapt();
    check_decode_bool_adapt();
}
