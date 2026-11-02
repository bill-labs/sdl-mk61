#include "microlib/microlib.h"
#include "microlib/image.h"
#include "microlib/clock.h"
#include "mkenv.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *mm_malloc(size_t size) { return malloc(size); }
void mm_free(void *ptr) { free(ptr); }
void *mm_wsalloc(size_t size) { return malloc(size); }
void mm_wsfree(void *ptr) { free(ptr); }

void fillareab(void *ptr, u8 value, size_t size) { memset(ptr, value, size); }

void fillaread(u32 *ptr, u32 value, size_t count)
{
	for (size_t i = 0; i < count; i++)
		ptr[i] = value;
}

void copyareab(void *dst, const void *src, size_t size) { memcpy(dst, src, size); }

u32 sn_scale32u(u32 value, u32 n, u32 d)
{
	if (!d)
		return 0;
	return (u32)(((u64)value * (u64)n + (d >> 1)) / d);
}

void sn_remdiv32u(u32 value, u32 divisor, u32 *q, u32 *r)
{
	if (!divisor)
	{
		if (q)
			*q = 0;
		if (r)
			*r = 0;
		return;
	}
	if (q)
		*q = value / divisor;
	if (r)
		*r = value % divisor;
}

static inline u32 to_argb(u32 pix)
{
	u8 b = (u8)(pix & 0xFF);
	u8 r = (u8)((pix >> 8) & 0xFF);
	u8 g = (u8)((pix >> 16) & 0xFF);
	u8 a = (u8)((pix >> 24) & 0xFF);
	return ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
}

static inline u8 channel(u32 c, u32 shift) { return (u8)((c >> shift) & 0xFF); }

void mg_image_copy(image_t *dst, const image_t *src)
{
	if (!dst || !src || !dst->width || !dst->height)
		return;

	u32 dw = dst->width;
	u32 dh = dst->height;
	u32 sw = src->width;
	u32 sh = src->height;

	for (u32 y = 0; y < dh; y++)
	{
		u32 fy = (u32)(((u64)y * (sh - 1) << 16) / (dh - 1 ? dh - 1 : 1));
		u32 sy = fy >> 16;
		u32 wy = fy & 0xFFFF;
		const u32 *sline0 = src->data + sy * sw;
		const u32 *sline1 = src->data + (sy + 1 < sh ? sy + 1 : sy) * sw;
		u32 *dline = dst->data + y * dw;
		for (u32 x = 0; x < dw; x++)
		{
			u32 fx = (u32)(((u64)x * (sw - 1) << 16) / (dw - 1 ? dw - 1 : 1));
			u32 sx = fx >> 16;
			u32 wx = fx & 0xFFFF;

			u32 c00 = to_argb(sline0[sx]);
			u32 c01 = to_argb(sline0[(sx + 1 < sw) ? sx + 1 : sx]);
			u32 c10 = to_argb(sline1[sx]);
			u32 c11 = to_argb(sline1[(sx + 1 < sw) ? sx + 1 : sx]);

			u32 w00 = ((0x10000 - wx) * (0x10000 - wy)) >> 16;
			u32 w01 = (wx * (0x10000 - wy)) >> 16;
			u32 w10 = ((0x10000 - wx) * wy) >> 16;
			u32 w11 = (wx * wy) >> 16;

			u32 a00 = channel(c00, 24);
			u32 a01 = channel(c01, 24);
			u32 a10 = channel(c10, 24);
			u32 a11 = channel(c11, 24);

			u64 a_acc = (u64)a00 * w00 + (u64)a01 * w01 + (u64)a10 * w10 + (u64)a11 * w11;
			u32 a = (u32)(a_acc >> 16);

			u32 r00 = channel(c00, 16) * a00;
			u32 r01 = channel(c01, 16) * a01;
			u32 r10 = channel(c10, 16) * a10;
			u32 r11 = channel(c11, 16) * a11;

			u32 g00 = channel(c00, 8) * a00;
			u32 g01 = channel(c01, 8) * a01;
			u32 g10 = channel(c10, 8) * a10;
			u32 g11 = channel(c11, 8) * a11;

			u32 b00 = channel(c00, 0) * a00;
			u32 b01 = channel(c01, 0) * a01;
			u32 b10 = channel(c10, 0) * a10;
			u32 b11 = channel(c11, 0) * a11;

			u32 r = (u32)(((u64)r00 * w00 + (u64)r01 * w01 + (u64)r10 * w10 + (u64)r11 * w11) >> 16);
			u32 g = (u32)(((u64)g00 * w00 + (u64)g01 * w01 + (u64)g10 * w10 + (u64)g11 * w11) >> 16);
			u32 b = (u32)(((u64)b00 * w00 + (u64)b01 * w01 + (u64)b10 * w10 + (u64)b11 * w11) >> 16);

			if (a)
			{
				r /= a;
				g /= a;
				b /= a;
			}
			else
			{
				r = g = b = 0;
			}

			dline[x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
}

void mg_image_draw(u32 x, u32 y, const image_t *image)
{
	if (!mk_env || !mk_env->vbuf || !image)
		return;

	u32 width = mk_env->mk_width;
	u32 height = mk_env->mk_height;

	for (u32 j = 0; j < image->height; j++)
	{
		u32 dy = y + j;
		if (dy >= height)
			continue;
		u32 *dst = mk_env->vbuf + dy * width + x;
		const u32 *src = image->data + j * image->width;
		for (u32 i = 0; i < image->width; i++)
		{
			u32 dx = x + i;
			if (dx >= width)
				continue;
			u32 pix = src[i]; /* image data already stored as ARGB */
			u32 a = (pix >> 24) & 0xFF;
			if (!a)
				continue;
			if (a == 0xFF)
			{
				dst[i] = pix;
			}
			else
			{
				u32 inv = 255 - a;
				u32 sr = (pix >> 16) & 0xFF;
				u32 sg = (pix >> 8) & 0xFF;
				u32 sb = pix & 0xFF;
				u32 dr = (dst[i] >> 16) & 0xFF;
				u32 dg = (dst[i] >> 8) & 0xFF;
				u32 db = dst[i] & 0xFF;
				u32 r = (sr * a + dr * inv + 127) / 255;
				u32 g = (sg * a + dg * inv + 127) / 255;
				u32 b = (sb * a + db * inv + 127) / 255;
				dst[i] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
			}
		}
		if (mk_env->vlines)
			mk_env->vlines[dy] = 0xFF;
	}
}

u64 mc_get_run_time(void)
{
	static u64 start_us = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	u64 now = (u64)tv.tv_sec * 1000000ULL + (u64)tv.tv_usec;
	if (!start_us)
		start_us = now;
	return now - start_us;
}
