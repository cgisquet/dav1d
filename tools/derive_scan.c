#define DAV1D_DERIVE_SCANS

#include "config.h"

#include <stdio.h>
#include "src/tables.c"
#include "src/scan.c"

int main(void)
{
  //static const char *TX_CLASSES[3] = { "TX_CLASS_2D", "TX_CLASS_V", "TX_CLASS_H" };
  static const char *SCAN_TYPE[3] = { "default", "mcol", "mrow" };
  static const char *TX_SIZES[N_RECT_TX_SIZES] = {
    // First square - TxfmSize
    "4X4", "8X8", "16X16", "32X32", "64X64",
    // then rect
    "4X8", "8X4", "8X16", "16X8", "16X32", "32X16", "32X64",
    "64X32", "4X16", "16X4", "8X32", "32X8", "16X64", "64X16",
  };

  for (int type = 0; type < 3; type++)
  for (int tx = 0; tx < N_RECT_TX_SIZES; tx++) {
    const TxfmInfo *const t_dim = dav1d_txfm_dimensions+tx;
    const int sw = imin(t_dim->w, 8), sh = imin(t_dim->h, 8);
    int num_coeffs = 16*sw*sh;
    int num_class = t_dim->w < 8 && t_dim->h < 8 && num_coeffs <= 256 ? 3 : 1;
    int psize = 0;
    do { psize++; num_coeffs /= 10; } while (num_coeffs);
    for (int tx_class = 0; tx_class < num_class; tx_class++) {
      const int16_t *const scan = dav1d_scans[tx][tx_class];
      ptrdiff_t stride = 4 * (sh + 1);
      const int shift = 2 + imin(t_dim->lh, 3), mask = 4 * sh - 1;
      int split = 0;

      if (tx_class == TX_CLASS_H) stride = 4 * (sw + 1);

      switch (type)
      {
      case 0:
      if (t_dim->w==16 || t_dim->h==16)
        continue;
      printf("static const scanpos ALIGN(av1_%s_scanpos_%s[], 32) = {\n",
             SCAN_TYPE[tx_class], TX_SIZES[tx]); break;
      case 1:
      split = t_dim->w==16 || t_dim->h==16;
      printf("static const uint8_t av1_%s_scanctx_%s[] = {\n",
             SCAN_TYPE[tx_class], TX_SIZES[tx]); break;
      case 2:
      if (t_dim->w==16 || t_dim->h==16)
        continue;
      printf("static const uint8_t av1_%s_scanbr_%s[] = {\n",
             SCAN_TYPE[tx_class], TX_SIZES[tx]); break;
      }
      
      if (1) //!split)
      for (int i = 0; i < 4*sh; i++) {
        for (int j = 0; j < 4*sw; j++) {
          int zz = j + 4*i*sw;
          if (zz > 32*32-1)
            fprintf(stderr, "Buggy for %ix%i at (%i,%i): %i\n",
                    4*t_dim->w, 4*t_dim->h, i, j, zz);
          int rc = scan[zz];
#if 1
          if (j && !(j&7)) printf("\n%.*s", j/2, "                            ");
          int x = rc >> shift, y = rc & mask;
          int offset = x * stride + y;
          int nz, br;
          switch (tx_class) {
          case TX_CLASS_2D:
            nz = !rc ? 0 : dav1d_nz_map_ctx_offset[tx][5*imin(y, 4)+ imin(x, 4)]; // rc=0=>ctx=0!
            br = y < 2 && x < 2 ? 7 : 14;
            break;
          case TX_CLASS_H:
            offset = y * stride + x;
            nz = 26 + imin(x, 2)*5;
            br = x == 0 ? 7 : 14;
            break;
          case TX_CLASS_V:
            nz = 26 + imin(y, 2)*5;
            br = y == 0 ? 7 : 14;
            break;
          }
          if (!rc) br = 0;
          switch (type)
          {
          case 0: printf(" {%*i,%*i},", psize, rc, psize, offset); break;
          case 1: printf(" %2i,", nz); break;
          case 2: printf(" %2i,", br); break;
          }
#else
          printf(" %*i,", psize, rc);
#endif
        }
        printf("\n");
      }
      else { // TX_CLASS_2D && scanctx => nz only
        int w = 4*sw;
        int eob = 4*sh*w;
        for (int i = 0; i < eob; i++)
        {
          int rc = scan[i];
          int x = rc >> shift, y = rc & mask;
          int nz = nz = !rc ? 0 : dav1d_nz_map_ctx_offset[tx][5*imin(y, 4)+ imin(x, 4)];
          if (rc && !(i&(w-1))) printf("\n");
          printf(" %2i,", nz);
        }
      }
      printf("};\n\n");
    }
  }

  return 1;
}
