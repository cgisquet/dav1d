#ifndef __DAV1D_SRC_SPLAT_MV_H__
#define __DAV1D_SRC_SPLAT_MV_H__

#include "common/attributes.h"
#include "src/levels.h"

typedef union aliasmv { refmvs rmv[4]; uint8_t u8[48]; } ATTR_ALIAS aliasmv;

#define decl_splat_mv_fn(name) \
void (name)(refmvs *r, ptrdiff_t stride, int bw4, int bh4, aliasmv *a)
typedef decl_splat_mv_fn(*splat_mv_fn);

typedef struct Dav1dSplatMVDSPContext {
    splat_mv_fn splat;
} Dav1dSplatMVDSPContext;

#if ARCH_X86_64 && HAVE_ASM
decl_splat_mv_fn(dav1d_splat_mv_sse);
#define SPLAT_MV(c, r, stride, bw4, bh4, a) dav1d_splat_mv_sse(r, stride, bw4, bh4, a)
#else
#define SPLAT_MV(c, r, stride, bw4, bh4, a) c->splat(c, r, stride, bw4, bh4, a)
#endif

void dav1d_splat_mv_init(Dav1dSplatMVDSPContext *const c);
void dav1d_splat_mv_init_x86(Dav1dSplatMVDSPContext *const c);

#endif // ~__DAV1D_SRC_SPLAT_MV_H__
