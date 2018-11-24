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

#define decl_msac_decode_bool_equi_fn(name) \
unsigned (name)(MsacContext *const s)
typedef decl_msac_decode_bool_equi_fn(*msac_decode_bool_equi_fn);

#define MAX_BYTES (100000)

unsigned dav1d_msac_decode_bool_equi(MsacContext *const s);

static unsigned char buf[MAX_BYTES];

static int msac_equal(MsacContext *a, MsacContext *b) {
    return a->buf_pos == b->buf_pos && a->buf_end == b->buf_end &&
        a->dif == b->dif && a->rng == b->rng && a->cnt == b->cnt;
}

static void check_decode_bool_equi() {
    int i;

    /* Create a random EC segment */
    for (i = 0; i < MAX_BYTES; i++) {
        buf[i] = rand() & 0xff;
    }

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

void checkasm_check_msac(void) {
    check_decode_bool_equi();
}
