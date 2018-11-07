#include "config.h"
#include <stdlib.h>

#include "common/attributes.h"
#include "common/intops.h"

#include "src/splat_mvs.h"

static NOINLINE decl_splat_mv_fn(splat_c)
{
    do {
        for (int x = 0; x < bw4; x++)
            r[x] = a->rmv[0];
        r += stride;
    } while (--bh4);
}

void dav1d_splat_mv_init(Dav1dSplatMVDSPContext *const c)
{
    c->splat = splat_c;
#if HAVE_ASM && ARCH_X86
    dav1d_splat_mv_init_x86(c);
#endif
}
