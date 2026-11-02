#include "microlib/microlib.h"
#include "microlib/heaps.h"
#include "microlib/clock.h"
#include "microlib/image.h"
#include "mkenv.h"

#include "console.h"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct {
	u32 w, h;
	u32 xa, ya;
	u32 r;
} row_meta_t;

typedef struct {
	u32 w, h;
	u32 xmin, y, xmax;
} angle_meta_t;

static const u32 BR0H = 455;
static const row_meta_t ROW_META[6] = {
	{2306, 455, 23, 244, 42},
	{2321, 373, 0, 249, 80},
	{2260, 381, 19, 257, 0},
	{2241, 370, 0, 246, 0},
	{2241, 349, 0, 225, 0},
	{2241, 349, 0, 225, 0},
};
static const angle_meta_t ANGLE_META = {896, 200, 289, 146, 769};

static u32 uround_pos(double v)
{
	return (u32)(v + 0.5);
}

u32 calc_button_event(u32 *px, u32 *py)
{
	u32 x = *px;
	u32 y = *py;
	u32 i, j;
	u32 byh = mk_env->button_height >> 1;
	u32 t, b, l, r;

	for (i = 0; i < 6; i++)
	{
		t = mk_env->buttonrows[i]->y_anchor - byh;
		b = mk_env->buttonrows[i]->y_anchor + byh;
		if ((y >= t) && (y <= b))
		{
			l = mk_env->buttonrows[i]->x_anchor;
			r = mk_env->buttonrows[i]->x_anchor + mk_env->button_width;
			for (j = 0; j < 5; j++)
			{
				if ((x >= l) && (x <= r))
				{
					*px = j;
					*py = 5 - i;
					return 1;
				}
				l += mk_env->button_step;
				r += mk_env->button_step;
			}
		}
	}
	return 0;
}

static void asbuttonpos(u32 *x, u32 *y)
{
	angleswitch_t *as = mk_env->angleswitch;
	image_t *button = (image_t *)((u8 *)as + as->button_off);

	*x = as->x_pos + as->button_x_min - (button->width >> 1) + as->button_pos * (as->button_x_max - as->button_x_min) / 100;
	*y = as->y_pos + as->button_y - (button->height >> 1);
}

void calc_powerbutton_event(s32 x, s32 y, u32 on)
{
	cgimage_t *pwr = mk_env->powerbutton;
	s32 px = (s32)pwr->x_pos;
	s32 py = (s32)pwr->y_pos;
	s32 pw = (s32)pwr->image.width;
	s32 ph = (s32)pwr->image.height;

	if (mk_env->flags & MK_POWERBUTTON_PRESSED)
	{
		if (!on)
		{
			if (!(mk_env->flags & MK_POWER_ON))
				mk_env->flags |= MK_POWER_ON;
			mk_env->flags |= MK_POWERBUTTON_RELEASED;
		}
	}

	if (on && (x >= px) && (y >= py) && (x < px + pw) && (y < py + ph))
	{
		mk_env->flags |= MK_POWERBUTTON_PRESSED;
		mk_env->flags &= ~MK_POWERBUTTON_RELEASED;
		mk_env->powerbutton_ts = mc_get_run_time();
	}
}

void calc_angleswitch_event(s32 x, s32 y, u32 on)
{
	angleswitch_t *as;
	image_t *button;
	u32 bx_u, by_u;
	s32 bx, by;
	s32 min, max;
	s32 pos;

	asbuttonpos(&bx_u, &by_u);
	bx = (s32)bx_u;
	by = (s32)by_u;
	if (mk_env->flags & MK_ANGLESWITCH_MOVING)
	{
		as = mk_env->angleswitch;
		button = (image_t *)((u8 *)as + as->button_off);
		pos = x;
		min = (s32)as->button_x_min + (s32)as->x_pos;
		max = (s32)as->button_x_max + (s32)as->x_pos;
		if (pos < min)
			pos = min;
		if (pos > max)
			pos = max;
		u32 pos_scaled = sn_scale32u((u32)(pos - min), 100, as->button_x_max - as->button_x_min);
		if (pos_scaled != as->button_pos)
		{
			as->button_pos = pos_scaled;
			mk_env->flags |= MK_ANGLESWITCH_UPDATED;
		}
		if (!on)
		{
			mk_env->flags &= ~MK_ANGLESWITCH_MOVING;
			pos = (s32)as->button_pos;
			if (pos <= 33)
				pos = 0;
			else if (pos >= 66)
				pos = 100;
			else
				pos = 50;
			if ((u32)pos != as->button_pos)
			{
				as->button_pos = (u32)pos;
				mk_env->flags |= MK_ANGLESWITCH_UPDATED;
			}
		}
	}
	else
	{
		if (!on || (x < bx) || (y < by))
			return;
		as = mk_env->angleswitch;
		button = (image_t *)((u8 *)as + as->button_off);
		bx += button->width;
		by += button->height;
		if ((x < bx) && (y < by))
			mk_env->flags |= MK_ANGLESWITCH_MOVING;
	}
}

void calc_draw_buttonrow(u32 row)
{
	buttonrow_t *brow = mk_env->buttonrows[row];
	mg_image_draw(brow->x_pos, brow->y_pos, &brow->image);
}

void calc_draw_powerbutton(void)
{
	cgimage_t *pwr = mk_env->powerbutton;
	mg_image_draw(pwr->x_pos, pwr->y_pos, &pwr->image);
}

void calc_draw_powerbutton_dim(void)
{
	cgimage_t *pwr = mk_env->powerbutton_dim ? mk_env->powerbutton_dim : mk_env->powerbutton;
	if (!pwr)
		return;
	mg_image_draw(pwr->x_pos, pwr->y_pos, &pwr->image);
}

void calc_draw_namelabel(void)
{
	cgimage_t *nl = mk_env->namelabel;
	mg_image_draw(nl->x_pos, nl->y_pos, &nl->image);
}

/* Draw a simple outline over the currently pressed button (if any) */
void calc_draw_pressed(void)
{
	if (!mk_env->pressed_active || mk_env->pressed_row < 0 || mk_env->pressed_row > 5)
		return;
	u32 row_idx = (u32)mk_env->pressed_row;
	buttonrow_t *brow = mk_env->buttonrows[row_idx];
	if (!brow)
		return;
	u32 x = brow->x_anchor + mk_env->pressed_col * mk_env->button_step;
	u32 y = brow->y_anchor - (mk_env->button_height >> 1);
	u32 w = mk_env->button_width;
	u32 h = mk_env->button_height;
	u32 c = 0xFF00FF00; /* green outline */

	/* Top and bottom borders */
	fillaread(mk_env->vbuf + y * mk_env->mk_width + x, c, w);
	fillaread(mk_env->vbuf + (y + h - 1) * mk_env->mk_width + x, c, w);
	/* Left and right borders */
	for (u32 j = 1; j + 1 < h; j++)
	{
		mk_env->vbuf[(y + j) * mk_env->mk_width + x] = c;
		mk_env->vbuf[(y + j) * mk_env->mk_width + x + w - 1] = c;
	}
}

void calc_invert_pressed(void)
{
	if (!mk_env->pressed_active || mk_env->pressed_row < 0 || mk_env->pressed_row > 5)
		return;
	u32 row_idx = (u32)mk_env->pressed_row;
	buttonrow_t *brow = mk_env->buttonrows[row_idx];
	if (!brow)
		return;
	u32 x = brow->x_anchor + mk_env->pressed_col * mk_env->button_step;
	u32 y = brow->y_anchor - (mk_env->button_height >> 1);
	u32 w = mk_env->button_width;
	u32 h = mk_env->button_height;

	u32 *p = mk_env->vbuf + y * mk_env->mk_width + x;
	for (u32 j = 0; j < h; j++)
	{
		u32 *row = p + j * mk_env->mk_width;
		for (u32 i = 0; i < w; i++)
		{
			u32 pix = row[i];
			u32 a = (pix >> 24) & 0xFF;
			if (!a)
				continue;
			u32 r = 0xFF - ((pix >> 16) & 0xFF);
			u32 g = 0xFF - ((pix >> 8) & 0xFF);
			u32 b = 0xFF - (pix & 0xFF);
			row[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
		if (mk_env->vlines)
			mk_env->vlines[y + j] = 0xFF;
	}
}

void calc_dim_pressed(void)
{
	if (!mk_env->pressed_active || mk_env->pressed_row < 0 || mk_env->pressed_row > 5)
		return;
	u32 row_idx = (u32)mk_env->pressed_row;
	buttonrow_t *brow = mk_env->buttonrows[row_idx];
	if (!brow)
		return;
	u32 x = brow->x_anchor + mk_env->pressed_col * mk_env->button_step;
	u32 y = brow->y_anchor - (mk_env->button_height >> 1);
	u32 w = mk_env->button_width;
	u32 h = mk_env->button_height;

	u32 *p = mk_env->vbuf + y * mk_env->mk_width + x;
	for (u32 j = 0; j < h; j++)
	{
		u32 *row = p + j * mk_env->mk_width;
		for (u32 i = 0; i < w; i++)
		{
			u32 pix = row[i];
			u32 a = (pix >> 24) & 0xFF;
			if (!a)
				continue;
			u32 r = (pix >> 16) & 0xFF;
			u32 g = (pix >> 8) & 0xFF;
			u32 b = pix & 0xFF;
			/* Darken by ~30% */
			r = (r * 7) / 10;
			g = (g * 7) / 10;
			b = (b * 7) / 10;
			row[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
		if (mk_env->vlines)
			mk_env->vlines[y + j] = 0xFF;
	}
}

void calc_dim_powerbutton(void)
{
	if (!(mk_env->flags & MK_POWERBUTTON_PRESSED) || !mk_env->powerbutton)
		return;
	cgimage_t *pwr = mk_env->powerbutton;
	u32 x = pwr->x_pos;
	u32 y = pwr->y_pos;
	u32 w = pwr->image.width;
	u32 h = pwr->image.height;

	u32 *p = mk_env->vbuf + y * mk_env->mk_width + x;
	for (u32 j = 0; j < h; j++)
	{
		u32 *row = p + j * mk_env->mk_width;
		for (u32 i = 0; i < w; i++)
		{
			u32 pix = row[i];
			u32 a = (pix >> 24) & 0xFF;
			if (!a)
				continue;
			u32 r = (pix >> 16) & 0xFF;
			u32 g = (pix >> 8) & 0xFF;
			u32 b = pix & 0xFF;
			r = (r * 7) / 10;
			g = (g * 7) / 10;
			b = (b * 7) / 10;
			row[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
		if (mk_env->vlines)
			mk_env->vlines[y + j] = 0xFF;
	}
}

void fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
	u32 *p = mk_env->vbuf + y * mk_env->mk_width + x;
	u8 *l = mk_env->vlines + y;
	u32 i;

	for (i = 0; i < h; i++)
	{
		fillaread(p, color, w);
		p += mk_env->mk_width;
		l[i] = 0xFF;
	}
}

void calc_draw_angleswitch(void)
{
	angleswitch_t *as = mk_env->angleswitch;
	image_t *button = (image_t *)((u8 *)as + as->button_off);
	u32 bx, by;

	asbuttonpos(&bx, &by);
	fill_rect(as->x_pos, as->y_pos, as->image.width, as->image.height, mk_env->bgcolor);
	mg_image_draw(as->x_pos, as->y_pos, &as->image);
	mg_image_draw(bx, by, button);
}

/* === Asset loading helpers (PNG via SDL_image) === */

static const char *asset_candidates[] = {
	DATADIR
};

static int resolve_asset_path(const char *file, char *out, size_t outsz)
{
	size_t total = sizeof(asset_candidates) / sizeof(asset_candidates[0]);
	for (size_t i = 0; i < total; i++)
	{
		int n = snprintf(out, outsz, "%s/%s", asset_candidates[i], file);
		if (n <= 0 || (size_t)n >= outsz)
			continue;
		FILE *f = fopen(out, "rb");
		if (f)
		{
			fclose(f);
			return 0;
		}
	}
	return 1;
}

static image_t *load_png_image(const char *file)
{
	char path[512];
	if (resolve_asset_path(file, path, sizeof(path)))
		return NULL;

	SDL_Surface *surf = IMG_Load(path);
	if (!surf)
		return NULL;

	SDL_Surface *fmt_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32,
												 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!fmt_surf)
	{
		SDL_FreeSurface(surf);
		return NULL;
	}
	SDL_Surface *conv = SDL_ConvertSurface(surf, fmt_surf->format, SDL_SWSURFACE);
	SDL_FreeSurface(fmt_surf);
	SDL_FreeSurface(surf);
	if (!conv)
		return NULL;

	u32 w = (u32)conv->w;
	u32 h = (u32)conv->h;
	size_t sz = sizeof(image_t) + (size_t)w * (size_t)h * sizeof(u32);
	image_t *img = mm_malloc(sz);
	if (!img)
	{
		SDL_FreeSurface(conv);
		return NULL;
	}
	img->width = w;
	img->height = h;

	u32 *dst = img->data;
	u8 *src = (u8 *)conv->pixels;
	int pitch = conv->pitch;
	SDL_PixelFormat *fmt = conv->format;
	for (u32 y = 0; y < h; y++)
	{
		u32 *row = (u32 *)(src + y * pitch);
		for (u32 x = 0; x < w; x++)
		{
			u8 r, g, b, a;
			SDL_GetRGBA(row[x], fmt, &r, &g, &b, &a);
			if (a && a < 255)
			{
				/* Premultiply to avoid light fringes on dark background */
				r = (u8)((r * a + 127) / 255);
				g = (u8)((g * a + 127) / 255);
				b = (u8)((b * a + 127) / 255);
			}
			*dst++ = ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | (u32)b;
		}
	}

	SDL_FreeSurface(conv);
	return img;
}

static buttonrow_t *load_buttonrow(const row_meta_t *meta, const char *png_name)
{
	u32 orig_w = meta->w;
	u32 orig_h = meta->h;
	u32 xa = meta->xa;
	u32 ya = meta->ya;
	buttonrow_t *brow = NULL;
	image_t *image = load_png_image(png_name);
	if (!image)
		return NULL;

	u32 w = image->width;
	u32 h = image->height;

	brow = mm_malloc(sizeof(buttonrow_t) + w * h * 4);
	if (brow)
	{
		brow->image.width = w;
		brow->image.height = h;
		brow->x_anchor = sn_scale32u(xa, w, orig_w);
		brow->y_anchor = sn_scale32u(ya, h, orig_h);
		copyareab(brow->image.data, image->data, (size_t)w * (size_t)h * sizeof(u32));
	}
	mm_free(image);
	return brow;
}

static cgimage_t *load_cgimage(const char *png_name)
{
	image_t *src = load_png_image(png_name);
	if (!src)
		return NULL;

	u32 w = src->width;
	u32 h = src->height;
	u32 size = sizeof(cgimage_t) + w * h * 4;
	cgimage_t *cgi = mm_malloc(size);
	if (cgi)
	{
		cgi->image.width = w;
		cgi->image.height = h;
		copyareab(cgi->image.data, src->data, (size_t)w * (size_t)h * sizeof(u32));
	}
	mm_free(src);
	return cgi;
}

static cgimage_t *make_dim_cgimage(const cgimage_t *src)
{
	if (!src)
		return NULL;

	u32 w = src->image.width;
	u32 h = src->image.height;
	u32 size = sizeof(cgimage_t) + w * h * 4;
	cgimage_t *dim = mm_malloc(size);
	if (!dim)
		return NULL;
	dim->image.width = w;
	dim->image.height = h;
	dim->x_pos = src->x_pos;
	dim->y_pos = src->y_pos;

	const u32 *s = src->image.data;
	u32 *d = dim->image.data;
	size_t count = (size_t)w * (size_t)h;
	for (size_t i = 0; i < count; i++)
	{
		u32 pix = s[i];
		u32 a = (pix >> 24) & 0xFF;
		u32 r = (pix >> 16) & 0xFF;
		u32 g = (pix >> 8) & 0xFF;
		u32 b = pix & 0xFF;
		r = (r * 7) / 10;
		g = (g * 7) / 10;
		b = (b * 7) / 10;
		d[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	return dim;
}

static angleswitch_t *load_angleswitch(const angle_meta_t *meta, u32 n, u32 d)
{
	(void)n;
	(void)d;
	image_t *sw, *button, *nsw, *nbutton;
	angleswitch_t *as;
	u32 iw, ih, bw, bh;
	u32 size, boff;
	sw = load_png_image("06.png");
	if (!sw)
		return NULL;
	as = NULL;
	button = load_png_image("07.png");
	if (!button)
		goto fail;

	iw = sw->width;
	ih = sw->height;
	bw = button->width;
	bh = button->height;
	boff = sizeof(angleswitch_t) + (iw * ih * 4);
	size = boff + sizeof(image_t) + (bw * bh * 4);
	as = mm_malloc(size);
	if (!as)
		goto fail1;
	nsw = &as->image;
	nbutton = (image_t *)((u8 *)as + boff);
	nsw->width = iw;
	nsw->height = ih;
	nbutton->width = bw;
	nbutton->height = bh;

	/* Place the knob relative to the pre-scaled track using original anchor data */
	as->button_x_min = sn_scale32u(meta->xmin, iw, meta->w);
	as->button_x_max = sn_scale32u(meta->xmax, iw, meta->w);
	as->button_y = sn_scale32u(meta->y, ih, meta->h);
	as->button_pos = 100;
	as->button_off = boff;
	copyareab(nsw->data, sw->data, (size_t)iw * (size_t)ih * sizeof(u32));
	copyareab(nbutton->data, button->data, (size_t)bw * (size_t)bh * sizeof(u32));

fail1:
	mm_free(button);
fail:
	mm_free(sw);
	return as;
}

static void release_buttonrows(void)
{
	u32 i;

	for (i = 0; i < 6; i++)
	{
		if (mk_env->buttonrows[i])
		{
			mm_free(mk_env->buttonrows[i]);
			mk_env->buttonrows[i] = NULL;
		}
	}
}

static u32 load_graphics(u32 width, u32 height, u32 xoff, u32 yoff)
{
	u32 br_w[6], br_wo[6], br_xa[6];
	u32 br_w_max, br_xa_max;
	u32 total_h, br_interval, br0h;
	u32 r, maxright;
	u32 asx, asy;
	s32 i;
	static const char *row_files[6] = {
		"00.png", "01.png", "02.png", "03.png", "04.png", "05.png"};

	for (i = 0; i < 6; i++)
		mk_env->buttonrows[i] = NULL;

	br_w_max = br_xa_max = 0;
	br0h = BR0H;
	maxright = 0;
	for (i = 0; i < 6; i++)
	{
		const row_meta_t *meta = &ROW_META[i];
		br_w[i] = meta->w;
		br_xa[i] = meta->xa;
		r = meta->r;
		if (r > maxright)
			maxright = r;
		br_wo[i] = br_w[i] - br_xa[i] - r; /* Align all rows to button start X */
		if (br_xa[i] > br_xa_max)
			br_xa_max = br_xa[i];
	}
	for (i = 0; i < 6; i++)
	{
		br_wo[i] += br_xa_max + maxright; /* Move all rows to maximum X offset */
		if (br_wo[i] > br_w_max)
			br_w_max = br_wo[i];
	}

	total_h = 0;
	for (i = 0; i < 6; i++)
	{
		const row_meta_t *meta = &ROW_META[i];
		mk_env->buttonrows[i] = load_buttonrow(meta, row_files[i]);
		if (!mk_env->buttonrows[i])
			goto fail;
		double scale = (double)mk_env->buttonrows[i]->image.width / (double)meta->w;
		br_w[i] = mk_env->buttonrows[i]->image.width;
		br_xa[i] = uround_pos((double)meta->xa * scale);
		r = uround_pos((double)meta->r * scale);
		total_h += mk_env->buttonrows[i]->image.height;
		br_wo[i] = br_w[i] - br_xa[i] - r;
		if (br_xa[i] > br_xa_max)
			br_xa_max = br_xa[i];
		if (r > maxright)
			maxright = r;
	}

	if (total_h > height)
		goto fail;
	br_interval = (height - total_h) / 5;

	for (i = 5; i >= 0; i--)
	{
		mk_env->buttonrows[i]->x_pos = xoff + sn_scale32u(br_xa_max - br_xa[i], width, br_w_max);
		mk_env->buttonrows[i]->y_pos = yoff;
		mk_env->buttonrows[i]->x_anchor += mk_env->buttonrows[i]->x_pos;
		mk_env->buttonrows[i]->y_anchor += mk_env->buttonrows[i]->y_pos;
		yoff += mk_env->buttonrows[i]->image.height + br_interval;
	}

	mk_env->button_width = sn_scale32u(361, width, br_w_max);
	mk_env->button_height = sn_scale32u(249, mk_env->buttonrows[0]->image.height, br0h);
	mk_env->button_step = sn_scale32u(470, width, br_w_max);

	mk_env->angleswitch = load_angleswitch(&ANGLE_META, width, br_w_max);
	if (!mk_env->angleswitch)
		goto fail;

	asx = xoff + mk_env->buttonrows[5]->image.width - mk_env->angleswitch->image.width;
	asy = mk_env->buttonrows[5]->y_pos - br_interval - (mk_env->buttonrows[5]->image.height >> 1);
	asy -= mk_env->angleswitch->image.height >> 1;
	mk_env->angleswitch->x_pos = asx;
	mk_env->angleswitch->y_pos = asy;

	mk_env->powerbutton = load_cgimage("08.png");
	if (!mk_env->powerbutton)
		goto fail1;
	mk_env->powerbutton->x_pos = xoff;
	mk_env->powerbutton->y_pos = mk_env->buttonrows[5]->y_pos - br_interval - mk_env->powerbutton->image.height;
	mk_env->powerbutton_dim = make_dim_cgimage(mk_env->powerbutton);
	if (mk_env->powerbutton_dim)
	{
		mk_env->powerbutton_dim->x_pos = mk_env->powerbutton->x_pos;
		mk_env->powerbutton_dim->y_pos = mk_env->powerbutton->y_pos;
	}

	mk_env->namelabel = load_cgimage("09.png");
	if (!mk_env->namelabel)
		goto fail2;
	mk_env->namelabel->x_pos = xoff + mk_env->powerbutton->image.width;
	mk_env->namelabel->y_pos = mk_env->buttonrows[5]->y_pos - br_interval - mk_env->powerbutton->image.height;
	mk_env->namelabel->x_pos += (mk_env->angleswitch->x_pos - mk_env->namelabel->x_pos - mk_env->namelabel->image.width) >> 1;

	mk_env->disp_w = mk_env->mk_uwidth - (mk_env->mk_uwidth >> 4);
	mk_env->disp_dwx = mk_env->disp_w / 12;
	mk_env->disp_dw = mk_env->disp_dwx - ((mk_env->d_thickness << 1) + mk_env->d_thickness);
	mk_env->disp_dh = mk_env->disp_dw << 1;
	if (!(mk_env->disp_dh & 1))
		mk_env->disp_dh--;
	mk_env->disp_h = mk_env->disp_dh + (mk_env->disp_dh >> 2);
	mk_env->disp_x = ((mk_env->mk_uwidth - mk_env->disp_w) >> 1) + mk_env->mk_x;
	mk_env->disp_y = (mk_env->namelabel->y_pos - mk_env->disp_h) >> 1;

	return 1;

fail2:
	if (mk_env->powerbutton_dim)
		mm_free(mk_env->powerbutton_dim);
	mm_free(mk_env->powerbutton);
fail1:
	mm_free(mk_env->angleswitch);
fail:
	release_buttonrows();
	return 0;
}

void calc_graphics_release(void)
{
	release_buttonrows();
	if (mk_env->angleswitch)
		mm_free(mk_env->angleswitch);
	if (mk_env->powerbutton)
		mm_free(mk_env->powerbutton);
	if (mk_env->powerbutton_dim)
		mm_free(mk_env->powerbutton_dim);
	if (mk_env->namelabel)
		mm_free(mk_env->namelabel);
	if (mk_env->img_inited)
	{
		IMG_Quit();
		mk_env->img_inited = 0;
	}
}

u32 calc_graphics_init(void)
{
	u32 uw, uh;
	u32 top, border;
	u32 q, r;

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		return 0;
	mk_env->img_inited = 1;

	uw = mk_env->mk_width;
	uh = sn_scale32u(uw, 480, 320);
	if (uh > mk_env->mk_height)
	{
		uh = mk_env->mk_height;
		uw = sn_scale32u(uh, 320, 480);
	}

	border = uw >> 5;
	top = sn_scale32u(uh, 292, 1000);

	mk_env->mk_uwidth = uw;
	mk_env->mk_uheight = uh;
	mk_env->mk_x = (mk_env->mk_width - uw) >> 1;
	mk_env->mk_y = (mk_env->mk_height - uh) >> 1;

	sn_remdiv32u(mk_env->mk_uwidth, 90, &q, &r);
	if (r > 45)
		q++;
	if (!(q & 1))
		q--;
	mk_env->d_thickness = q;

	if (!load_graphics(uw - (border << 1), uh - top - border, mk_env->mk_x + border, mk_env->mk_y + top))
		goto fail_img;

	return 1;

fail_img:
	if (mk_env->img_inited)
	{
		IMG_Quit();
		mk_env->img_inited = 0;
	}
	return 0;
}
