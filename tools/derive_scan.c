#include <stdio.h>
#include "src/tables.c"
#include "src/scan.c"

int main(void)
{
  static const char *TX_CLASSES[3] = { "TX_CLASS_2D", "TX_CLASS_V", "TX_CLASS_H" };
  static const char *SCAN_TYPE[3] = { "default", "mcol", "mrow" };
  static const char *TX_SIZES[N_RECT_TX_SIZES] = {
    // First square - TxfmSize
    "4X4", "8X8", "16X16", "32X32", "64X64",
    // then rect
    "4X8", "8X4", "8X16", "16X8", "16X32", "32X16", "32X64",
    "64X32", "4X16", "16X4", "8X32", "32X8", "16X64", "64X16",
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
      const int16_t *const scan = dav1d_scans[tx][tx_class];
      const ptrdiff_t stride = 4 * (imin(t_dim->h, 8) + 1);
      const int shift = 2 + imin(t_dim->lh, 3), mask = 4 * imin(t_dim->h, 8) - 1;

      printf("static const scanpos ALIGN(av1_%s_scanpos_%s[], 32) = {\n",
             SCAN_TYPE[tx_class], TX_SIZES[tx]);
      for (int i = 0; i < 4*t_dim->h; i++) {
        for (int j = 0; j < 4*t_dim->w; j++) {
          int zz = j + i*4*t_dim->w;
          int rc = scan[zz];
#if 1
          if (j && !(j&7)) printf("\n%.*s", j/2, "                            ");
          int x = rc >> shift, y = rc & mask;
          int offset = x * stride + y;
          printf(" {%*i,%*i,%2i,%2i},", psize, rc, psize, offset, x, y);
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
