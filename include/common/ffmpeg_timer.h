/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * high precision timer, useful to profile code
 */

#ifndef AVUTIL_TIMER_H
#define AVUTIL_TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define FF_TIMER_UNITS "decicycles"
#define AV_READ_TIME read_time

static inline uint64_t read_time(void)
{
    uint32_t a, d;
    __asm__ volatile(
#if 1
                     "lfence \n\t"
#endif
                     "rdtsc  \n\t"
                     : "=a" (a), "=d" (d));
    return ((uint64_t)d << 32) + a;
}

static inline uint32_t av_log2(const uint32_t x) {
  uint32_t y;
  __asm__ ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}

#define START_TIMER                             \
    uint64_t tend;                              \
    uint64_t tstart = AV_READ_TIME();           \

#define STOP_TIMER(id)                                                    \
    tend = AV_READ_TIME();                                                \
    {                                                                     \
        static uint64_t tsum   = 0;                                       \
        static int tcount      = 0;                                       \
        static int tskip_count = 0;                                       \
        static int thistogram[32] = {0};                                  \
        thistogram[av_log2(tend - tstart)]++;                             \
        if (tcount < 2                        ||                          \
            tend - tstart < 8 * tsum / tcount ||                          \
            tend - tstart < 2000) {                                       \
            tsum+= tend - tstart;                                         \
            tcount++;                                                     \
        } else                                                            \
            tskip_count++;                                                \
        if (((tcount + tskip_count) & (tcount + tskip_count - 1)) == 0) { \
            fprintf(stderr, "%7"PRIu64" " FF_TIMER_UNITS " in %s,%8d runs,%7d skips",       \
                   tsum * 10 / tcount, id, tcount, tskip_count);          \
            fprintf(stderr, "\n");                             \
        }                                                                 \
    }

#endif /* AVUTIL_TIMER_H */
