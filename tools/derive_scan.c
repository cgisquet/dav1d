#include "config.h"
#include <stdio.h>

#define BUILD_SCANPOS

#include "src/tables.c"
#include "src/scan.c"

int main(void)
{
    //static const char *TX_CLASSES[3] = { "TX_CLASS_2D", "TX_CLASS_V", "TX_CLASS_H" };
    static const char *SCAN_TYPE[3] = { "default", "mcol", "mrow" };
    static const char *TX_SIZES[N_RECT_TX_SIZES] = {
        // First square - TxfmSize
        "4x4", "8x8", "16x16", "32x32", "64x64",
        // then rect
        "4x8", "8x4", "8x16", "16x8", "16x32", "32x16", "32x64",
        "64x32", "4x16", "16x4", "8x32", "32x8", "16x64", "64x16",
    };

    for (int tx = 0; tx < N_RECT_TX_SIZES; tx++) {
        const TxfmInfo *const t_dim = dav1d_txfm_dimensions+tx;
        if (t_dim->w==16 || t_dim->h==16)
            continue;
        int num_coeffs = 16*t_dim->w*t_dim->h;
        int num_class = t_dim->w < 8 && t_dim->h < 8 && num_coeffs <= 256 ? 3 : 1;
        int psize = 0;
        do { psize++; num_coeffs /= 10; } while (num_coeffs);
        for (int tx_class = 0; tx_class < num_class; tx_class++) {
            const uint16_t *const scan = dav1d_scans[tx][tx_class];
            const int sh = imin(t_dim->h, 8);
            const unsigned shift = 2 + imin(t_dim->lh, 3), mask = 4 * sh - 1;
            ptrdiff_t stride = tx_class == TX_CLASS_2D ? 4 * sh : 16;

            printf("static const scanpos ALIGN(av1_%s_scanpos_%s[], 32) = {\n",
                   SCAN_TYPE[tx_class], TX_SIZES[tx]);
            for (int i = 0; i < 4*t_dim->h; i++) {
                for (int j = 0; j < 4*t_dim->w; j++) {
                    int rc, zz = j + i*4*t_dim->w;
#if 1
                    int x, y, offset, nz, br;
                    if (j && !(j&7)) printf("\n%.*s", j/2, "                            ");
                    if (tx_class == TX_CLASS_H) {
                        rc = zz;
                        x = rc & mask;
                        y = rc >> shift;
                    } else {
                        rc = scan[zz];
                        x = rc >> shift;
                        y = rc & mask;
                    }
                    offset = x * stride + y;
                    switch (tx_class) {
                    case TX_CLASS_2D:
                        nz = 5*umin(y, 4)+ umin(x, 4); // rc=0=>ctx=0!
                        br = (x|y) > 1 ? 14 : 7;
                        break;
                    default:
                        nz = 26 + (y > 1 ? 10 : y * 5);
                        br = y == 0 ? 7 : 14;
                        break;
                    }
                    if (!rc) br = 0;
                    printf(" {%*i,%*i,%2i,%2i},", psize, rc, psize, offset, nz, br);
#else
                    printf(" %*i,", psize, rc);
#endif
                }
                printf("\n");
            }
            printf("};\n\n");
        }
    }

    return 1;
}
