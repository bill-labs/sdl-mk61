#ifndef MICROLIB_SHIM_H
#define MICROLIB_SHIM_H

#include <stddef.h>

/* Provide fixed-size types without relying on stdint.h (not present in GCC 3/4 toolchains here). */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;

#ifdef __cplusplus
extern "C" {
#endif

#define U16LEO(p, off) ((u16)((p)[off]) | ((u16)((p)[(off) + 1]) << 8))
#define U32LEO(p, off) ((u32)((p)[off]) | ((u32)((p)[(off) + 1]) << 8) | ((u32)((p)[(off) + 2]) << 16) | ((u32)((p)[(off) + 3]) << 24))

void fillareab(void *ptr, u8 value, size_t size);
void fillaread(u32 *ptr, u32 value, size_t count);
void copyareab(void *dst, const void *src, size_t size);

u32 sn_scale32u(u32 value, u32 n, u32 d);
void sn_remdiv32u(u32 value, u32 divisor, u32 *q, u32 *r);

#ifdef __cplusplus
}
#endif

#endif /* MICROLIB_SHIM_H */
