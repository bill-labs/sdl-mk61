#ifndef MICROLIB_HEAPS_SHIM_H
#define MICROLIB_HEAPS_SHIM_H

#include "microlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_wsalloc(size_t size);
void mm_wsfree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* MICROLIB_HEAPS_SHIM_H */
