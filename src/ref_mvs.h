/*
 * Copyright (c) 2001-2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef DAV1D_SRC_REF_MVS_H
#define DAV1D_SRC_REF_MVS_H

#include <stddef.h>

#include "src/levels.h"
#include "src/tables.h"

typedef struct refmvs {
    mv mv[2];
    int8_t ref[2]; // [0] = 0: intra=1, [1] = -1: comp=0
    int8_t mode, sb_type;
} refmvs;

typedef struct candidate_mv {
    mv this_mv;
    mv comp_mv;
    int weight;
} candidate_mv;

typedef struct AV1_COMMON AV1_COMMON;

// call once per frame thread
AV1_COMMON *dav1d_alloc_ref_mv_common(void);
void dav1d_free_ref_mv_common(AV1_COMMON *cm);

// call once per frame
int dav1d_init_ref_mv_common(AV1_COMMON *cm, int w8, int h8,
                             ptrdiff_t stride, int allow_sb128,
                             refmvs *cur, refmvs *ref_mvs[7],
                             unsigned cur_poc,
                             const unsigned ref_poc[7],
                             const unsigned ref_ref_poc[7][7],
                             const Dav1dWarpedMotionParams gmv[7],
                             int allow_hp, int force_int_mv,
                             int allow_ref_frame_mvs, int order_hint);

// call for start of each sbrow per tile
void dav1d_init_ref_mv_tile_row(AV1_COMMON *cm,
                                int tile_col_start4, int tile_col_end4,
                                int row_start4, int row_end4);

// call for each block
void dav1d_find_ref_mvs(candidate_mv *mvstack, int *cnt, mv (*mvlist)[2],
                        int *ctx, int refidx[2], int w4, int h4,
                        enum BlockSize bs, enum BlockPartition bp,
                        int by4, int bx4, int tile_col_start4,
                        int tile_col_end4, int tile_row_start4,
                        int tile_row_end4, AV1_COMMON *cm);

extern const uint8_t dav1d_bs_to_sbtype[];
extern const uint8_t dav1d_sbtype_to_bs[];

typedef union aliasmv { refmvs rmv[4]; uint8_t u8[48]; } ATTR_ALIAS aliasmv;

#include <smmintrin.h>

#define STOREU16(a, b) _mm_storeu_si128((__m128i *)(a), b);
#define STORE16(a, b)  _mm_store_si128 ((__m128i *)(a), b);
#define STORE8(a, b)   _mm_storel_epi64((__m128i *)(a), b);
#define LOAD16(a)      _mm_load_si128  ((__m128i *)(a));

static inline void splat_mv(refmvs *r, const ptrdiff_t stride,
                            const int bw4, int bh4, aliasmv *a)
{
    if (bw4 == 1) {
        const __m128i ref = LOAD16(a);
        register uint32_t val = *(uint32_t*)(a->u8+ 8);
        do {
            STORE8(r, ref);
            *(uint32_t*)(((aliasmv*)r)->u8+ 8) = val;
            //*(uint32_t*)(((aliasmv*)r)->u8+ 8) = _mm_extract_epi32(ref, 3);
            r += stride;
        } while (--bh4);
        return;
    }

    a->rmv[1] = a->rmv[0];
    if (bw4 == 2) {
        const __m128i ref1 = LOAD16(a->u8+ 0);
        const __m128i ref2 = _mm_loadl_epi64((__m128i *)(a->u8+16));
        do {
            aliasmv *dst = (aliasmv*)r;
            STOREU16(r, ref1);
            STORE8(dst->u8+16, ref2);
            r += stride;
        } while (--bh4);
        return;
    }

    const __m128i ref1 = LOAD16(a->u8+ 0);
    STOREU16(a->u8+24, ref1);
    __m128i tmp = _mm_loadl_epi64((__m128i *)(a->u8+4));
    STORE8(a->u8+40, tmp);
    const __m128i ref2 = LOAD16(a->u8+16);
    const __m128i ref3 = LOAD16(a->u8+32);
    do {
        aliasmv *dst = (aliasmv*)r;
        switch(bw4)
        {
        case 32:
            STORE16(dst->u8+368, ref3);
            STORE16(dst->u8+352, ref2);
            STORE16(dst->u8+336, ref1);
            STORE16(dst->u8+320, ref3);
            STORE16(dst->u8+304, ref2);
            STORE16(dst->u8+288, ref1);
            STORE16(dst->u8+272, ref3);
            STORE16(dst->u8+256, ref2);
            STORE16(dst->u8+240, ref1);
            STORE16(dst->u8+224, ref3);
            STORE16(dst->u8+208, ref2);
            STORE16(dst->u8+192, ref1);
            /* fall-thru */
        case 16:
            STORE16(dst->u8+176, ref3);
            STORE16(dst->u8+160, ref2);
            STORE16(dst->u8+144, ref1);
            STORE16(dst->u8+128, ref3);
            STORE16(dst->u8+112, ref2);
            STORE16(dst->u8+ 96, ref1);
            /* fall-thru */
        case 8:
            STORE16(dst->u8+ 80, ref3);
            STORE16(dst->u8+ 64, ref2);
            STORE16(dst->u8+ 48, ref1);
            /* fall-thru */
        case 4:
            STORE16(dst->u8+ 32, ref3);
            STORE16(dst->u8+ 16, ref2);
            STORE16(dst->u8+  0, ref1);
        }
        r += stride;
    } while (--bh4);
}

static inline void splat_oneref_mv(refmvs *r, const ptrdiff_t stride,
                                   const int by4, const int bx4,
                                   const enum BlockSize bs,
                                   const enum InterPredMode mode,
                                   const int ref, const mv mv,
                                   const int is_interintra)
{
    const int bw4 = dav1d_block_dimensions[bs][0];
    int bh4 = dav1d_block_dimensions[bs][1];

    r += by4 * stride + bx4;

    aliasmv ALIGN(a, 32);

    a.rmv[0] = (refmvs) {
        .ref = { ref + 1, is_interintra ? 0 : -1 },
        .mv = { mv },
        .sb_type = dav1d_bs_to_sbtype[bs],
        .mode = N_INTRA_PRED_MODES + mode,
    };

    splat_mv(r, stride, bw4, bh4, &a);
}

static inline void splat_intrabc_mv(refmvs *r, const ptrdiff_t stride,
                                    const int by4, const int bx4,
                                    const enum BlockSize bs, const mv mv)
{
    const int bw4 = dav1d_block_dimensions[bs][0];
    int bh4 = dav1d_block_dimensions[bs][1];

    r += by4 * stride + bx4;

    aliasmv ALIGN(a, 32);

    a.rmv[0] = (refmvs) {
        .ref = { 0, -1 },
        .mv = { mv },
        .sb_type = dav1d_bs_to_sbtype[bs],
        .mode = DC_PRED,
    };

    splat_mv(r, stride, bw4, bh4, &a);
}

static inline void splat_tworef_mv(refmvs *r, const ptrdiff_t stride,
                                   const int by4, const int bx4,
                                   const enum BlockSize bs,
                                   const enum CompInterPredMode mode,
                                   const int ref1, const int ref2,
                                   const mv mv1, const mv mv2)
{
    const int bw4 = dav1d_block_dimensions[bs][0];
    int bh4 = dav1d_block_dimensions[bs][1];

    r += by4 * stride + bx4;

    aliasmv ALIGN(a, 32);

    a.rmv[0] = (refmvs) {
        .ref = { ref1 + 1, ref2 + 1 },
        .mv = { mv1, mv2 },
        .sb_type = dav1d_bs_to_sbtype[bs],
        .mode = N_INTRA_PRED_MODES + N_INTER_PRED_MODES + mode,
    };

    splat_mv(r, stride, bw4, bh4, &a);
}

static inline void splat_intraref(refmvs *r, const ptrdiff_t stride,
                                  const int by4, const int bx4,
                                  const enum BlockSize bs,
                                  const enum IntraPredMode mode)
{
    const int bw4 = dav1d_block_dimensions[bs][0];
    int bh4 = dav1d_block_dimensions[bs][1];

    r += by4 * stride + bx4;

    aliasmv ALIGN(a, 32);

    a.rmv[0] = (refmvs) {
        .ref = { 0, -1 },
        .mv = { [0] = { .y = -0x8000, .x = -0x8000 }, },
        .sb_type = dav1d_bs_to_sbtype[bs],
        .mode = mode,
    };

    splat_mv(r, stride, bw4, bh4, &a);
}

static inline void fix_mv_precision(const Dav1dFrameHeader *const hdr,
                                    mv *const mv)
{
    if (hdr->force_integer_mv) {
        const int xmod = mv->x & 7;
        mv->x &= ~7;
        mv->x += (xmod > 4 - (mv->x < 0)) << 3;
        const int ymod = mv->y & 7;
        mv->y &= ~7;
        mv->y += (ymod > 4 - (mv->y < 0)) << 3;
    } else if (!hdr->hp) {
        if (mv->x & 1) {
            if (mv->x < 0) mv->x++;
            else           mv->x--;
        }
        if (mv->y & 1) {
            if (mv->y < 0) mv->y++;
            else           mv->y--;
        }
    }
}

#endif /* DAV1D_SRC_REF_MVS_H */
