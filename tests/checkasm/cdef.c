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

#include "tests/checkasm/checkasm.h"

#include <string.h>
#include <stdio.h>

#include "common/dump.h"

#include "src/levels.h"
#include "src/cdef.h"

static void init_tmp(pixel *buf, int n, const int bitdepth_max) {
    while (n--)
        *buf++ = rnd() & bitdepth_max;
}

static void check_cdef_filter(const cdef_fn fn, const int w, const int h,
                              const char *const name, int type)
{
    ALIGN_STK_32(pixel, src, 10 * 16 + 8, );
    ALIGN_STK_32(pixel, c_src, 10 * 16 + 8, ), *const c_src_ptr = c_src + 8;
    ALIGN_STK_32(pixel, a_src, 10 * 16 + 8, ), *const a_src_ptr = a_src + 8;
    ALIGN_STK_32(pixel, top, 16 * 2 + 8, ), *const top_ptr = top + 8;
    pixel left[8][2];

    declare_func(void, pixel *dst, ptrdiff_t dst_stride, const pixel (*left)[2],
                 pixel *const top[2], int pri_strength, int sec_strength,
                 int dir, int damping, enum CdefEdgeFlags edges HIGHBD_DECL_SUFFIX);

    if (check_func(fn, "%s_%dbpc", name, BITDEPTH)) {
        for (int dir = 0; dir < 8; dir++) {
            for (enum CdefEdgeFlags edges = 0; edges <= 0xf; edges++) {
#if BITDEPTH == 16
                const int bitdepth_max = rnd() & 1 ? 0x3ff : 0xfff;
#else
                const int bitdepth_max = 0xff;
#endif
                const int bitdepth_min_8 = bitdepth_from_max(bitdepth_max) - 8;
                init_tmp(src, 10 * 16 + 8, bitdepth_max);
                init_tmp(top, 16 * 2 + 8, bitdepth_max);
                init_tmp((pixel *) left,8 * 2, bitdepth_max);

                memcpy(a_src, src, (10 * 16 + 8) * sizeof(pixel));
                memcpy(c_src, src, (10 * 16 + 8) * sizeof(pixel));

                const int lvl = 1 + (rnd() % 62);
                const int damping = 3 + (rnd() & 3) + bitdepth_min_8 - (w == 4 || (rnd() & 1));
                const int pri_strength = (lvl >> 2) << bitdepth_min_8;
                int sec_strength = lvl & 3;
                sec_strength += sec_strength == 3;
                sec_strength <<= bitdepth_min_8;
                call_ref(c_src_ptr, 16 * sizeof(pixel), left,
                         (pixel *[2]) { top_ptr, top_ptr + 16 },
                         pri_strength, sec_strength, dir, damping, edges
                         HIGHBD_TAIL_SUFFIX);
                call_new(a_src_ptr, 16 * sizeof(pixel), left,
                         (pixel *[2]) { top_ptr, top_ptr + 16 },
                         pri_strength, sec_strength, dir, damping, edges
                         HIGHBD_TAIL_SUFFIX);
                checkasm_check_pixel(c_src, 16 * sizeof(pixel),
                                     a_src, 16 * sizeof(pixel),
                                     16, 10, "src");
                checkasm_check_pixel(c_src + 16 * 10, 16 * sizeof(pixel),
                                     a_src + 16 * 10, 16 * sizeof(pixel),
                                     8, 1, "src last row");
                bench_new(a_src_ptr, 16 * sizeof(pixel), left,
                          (pixel *[2]) { top_ptr, top_ptr + 16 },
                          pri_strength, sec_strength, dir, damping, edges
                          HIGHBD_TAIL_SUFFIX);
            }
        }
    }
    report(name);
}

static void check_cdef_direction(const cdef_dir_fn fn) {
    ALIGN_STK_32(pixel, src, 8 * 8,);

    declare_func(int, pixel *src, ptrdiff_t dst_stride, unsigned *var
                 HIGHBD_DECL_SUFFIX);

    if (check_func(fn, "cdef_dir_%dbpc", BITDEPTH)) {
        unsigned c_var, a_var;
#if BITDEPTH == 16
        const int bitdepth_max = rnd() & 1 ? 0x3ff : 0xfff;
#else
        const int bitdepth_max = 0xff;
#endif
        init_tmp(src, 64, bitdepth_max);

        const int c_dir = call_ref(src, 8 * sizeof(pixel), &c_var HIGHBD_TAIL_SUFFIX);
        const int a_dir = call_new(src, 8 * sizeof(pixel), &a_var HIGHBD_TAIL_SUFFIX);
        if (c_var != a_var || c_dir != a_dir) {
            if (fail()) {
                hex_fdump(stderr, src, 8 * sizeof(pixel), 8, 8, "src");
                fprintf(stderr, "c_dir %d a_dir %d\n", c_dir, a_dir);
            }
        }
        bench_new(src, 8 * sizeof(pixel), &a_var HIGHBD_TAIL_SUFFIX);
    }
    report("cdef_dir");
}

void bitfn(checkasm_check_cdef)(void) {
    Dav1dCdefDSPContext c;

    bitfn(dav1d_cdef_dsp_init)(&c);

    check_cdef_direction(c.dir);
    check_cdef_filter(c.fb[0], 8, 8, "cdef_filter_8x8", 0);
    check_cdef_filter(c.fb[1], 4, 8, "cdef_filter_4x8", 0);
    check_cdef_filter(c.fb[2], 4, 4, "cdef_filter_4x4", 0);
}
