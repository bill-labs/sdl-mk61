#ifndef MICROLIB_IMAGE_SHIM_H
#define MICROLIB_IMAGE_SHIM_H

#include "microlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct image_t
{
	u32 width;
	u32 height;
	u32 data[0];
} image_t;

void mg_image_copy(image_t *dst, const image_t *src);
void mg_image_draw(u32 x, u32 y, const image_t *image);

#ifdef __cplusplus
}
#endif

#endif /* MICROLIB_IMAGE_SHIM_H */
