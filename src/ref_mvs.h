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

static inline void AV_COPY64(void *d, const void *s)
{
    __asm__("movq   %1, %%mm0  \n\t"
            "movq   %%mm0, %0  \n\t"
            : "=m"(*(uint64_t*)d)
            : "m" (*(const uint64_t*)s)
            : "mm0");
}

static inline void AV_COPY128(void *d, const void *s)
{
    struct v {uint64_t v[2];};

    __asm__("movaps   %1, %%xmm0  \n\t"
            "movups   %%xmm0, %0  \n\t"
            : "=m"(*(struct v*)d)
            : "m" (*(const struct v*)s)
            : "xmm0");
}

typedef union aliasmv { refmvs rmv[4]; uint8_t u8[48]; } ATTR_ALIAS aliasmv;

static inline void splat_mv(refmvs *r, const ptrdiff_t stride,
                            const int bw4, int bh4, aliasmv *a)
{
    if (bw4 == 1) {
        do {
            AV_COPY64(((aliasmv*)r)->u8+ 0, a->u8+ 0);
            *(uint32_t*)(((aliasmv*)r)->u8+ 8) = *(uint32_t*)(a->u8+ 8);
            *r = a->rmv[0];
            r += stride;
        } while (--bh4);
        return;
    }

    a->rmv[1] = a->rmv[0];
    if (bw4 == 2) {
        do {
            AV_COPY128(((aliasmv*)r)->u8+ 0, a->u8+ 0);
            AV_COPY64 (((aliasmv*)r)->u8+16, a->u8+16);
            r += stride;
        } while (--bh4);
        return;
    }

    AV_COPY128(a->u8+24, a->u8+ 0); AV_COPY64 (a->u8+40, a->u8+ 4);
    do {
        aliasmv *dst = (aliasmv*)r;
        switch(bw4)
        {
        case 32:
            AV_COPY128(dst->u8+368, a->u8+32);
            AV_COPY128(dst->u8+352, a->u8+16);
            AV_COPY128(dst->u8+336, a->u8+ 0);
            AV_COPY128(dst->u8+320, a->u8+32);
            AV_COPY128(dst->u8+304, a->u8+16);
            AV_COPY128(dst->u8+288, a->u8+ 0);
            AV_COPY128(dst->u8+272, a->u8+32);
            AV_COPY128(dst->u8+256, a->u8+16);
            AV_COPY128(dst->u8+240, a->u8+ 0);
            AV_COPY128(dst->u8+224, a->u8+32);
            AV_COPY128(dst->u8+208, a->u8+16);
            AV_COPY128(dst->u8+192, a->u8+ 0);
            /* fall-thru */
        case 16:
            AV_COPY128(dst->u8+176, a->u8+32);
            AV_COPY128(dst->u8+160, a->u8+16);
            AV_COPY128(dst->u8+144, a->u8+ 0);
            AV_COPY128(dst->u8+128, a->u8+32);
            AV_COPY128(dst->u8+112, a->u8+16);
            AV_COPY128(dst->u8+ 96, a->u8+ 0);
            /* fall-thru */
        case 8:
            AV_COPY128(dst->u8+ 80, a->u8+32);
            AV_COPY128(dst->u8+ 64, a->u8+16);
            AV_COPY128(dst->u8+ 48, a->u8+ 0);
            /* fall-thru */
        case 4:
            AV_COPY128(dst->u8+ 32, a->u8+32);
            AV_COPY128(dst->u8+ 16, a->u8+16);
            AV_COPY128(dst->u8+  0, a->u8+ 0);
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
