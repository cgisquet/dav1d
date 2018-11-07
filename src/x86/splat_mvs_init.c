#include "src/cpu.h"
#include "src/splat_mvs.h"

decl_splat_mv_fn(dav1d_splat_mv_avx);

void dav1d_splat_mv_init_x86(Dav1dSplatMVDSPContext *const c) {
    const unsigned flags = dav1d_get_cpu_flags();

    if (flags & DAV1D_X86_CPU_FLAG_AVX)
        c->splat = dav1d_splat_mv_sse;

#if ARCH_X86_64
    if (!(flags & DAV1D_X86_CPU_FLAG_AVX)) return;
    //c->splat = dav1d_splat_mv_avx2;
#endif
}
