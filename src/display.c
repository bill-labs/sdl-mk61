#include "microlib/microlib.h"
#include "mkenv.h"

#include "console.h"

/* 7-segment digital display */

#define SEG_A		0x01
#define SEG_B		0x02
#define SEG_C		0x04
#define SEG_D		0x08
#define SEG_E		0x10
#define SEG_F		0x20
#define SEG_G		0x40

static u8 digitmask[] = { 
	/* 0 */	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
	/* 1 */	SEG_B | SEG_C,
	/* 2 */	SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,
	/* 3 */	SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
	/* 4 */	SEG_F | SEG_B | SEG_G | SEG_C,
	/* 5 */	SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,
	/* 6 */	SEG_A | SEG_F | SEG_G | SEG_C | SEG_D | SEG_E,
	/* 7 */	SEG_A | SEG_B | SEG_C,
	/* 8 */	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	/* 9 */	SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
	/* - */	SEG_G,
	/* L */	SEG_F | SEG_E | SEG_D,
	/* C */	SEG_A | SEG_F | SEG_E | SEG_D,
	/* Г */	SEG_A | SEG_F | SEG_E,
	/* E */	SEG_A | SEG_F | SEG_E | SEG_D | SEG_G,
	/*   */	0
};

/* Draw single vertical display segment */
static void draw_vseg(u32 *p, u32 sw, u32 len, u32 dir)
{
	u32 i, j;
	s32 off, offstep;
	u32 *pp;

	if (dir)
	{
		off = 1 + (mk_env->d_thickness - 1);
		offstep = -1;
	}
	else
	{
		off = 1;
		offstep = 1;
	}

	for (i = 0; i < mk_env->d_thickness; i++)
	{
		pp = p + i + off * sw;
		for (j = 0; j < len - (off << 1); j++)
		{
			*pp = mk_env->digit_color;
			pp += sw;
		}
		off += offstep;
	}
}

/* Draw single horizontal display segment */
static void draw_hseg(u32 *p, u32 sw, u32 len, u32 dir)
{
	u32 i;
	s32 off, offstep;
	
	switch (dir)
	{
		case 0:
			off = 1 + ((mk_env->d_thickness - 1) >> 1);
			offstep = -1;
			break;
		case 1:
			off = 1;
			offstep = 1;
			break;
		case 2:
			off = 1 + (mk_env->d_thickness - 1);
			offstep = -1;
			break;
	}
	
	for (i = 0; i < mk_env->d_thickness; i++)
	{
		fillaread(p + off, mk_env->digit_color, len - (off << 1));
		p += sw;
		if (!dir && (off == 1))
			offstep = 1;
		off += offstep;
	}
}

/* Draw segments listed in bitmask */
static void draw_segs(u32 *p, u32 sw, u32 w, u32 h, u32 segs)
{
	u32 hh = h >> 1;

	if (segs & 0x01)		/* A */
		draw_hseg(p, sw, w, 1);
	if (segs & 0x40)		/* G */
		draw_hseg(p + sw * (hh - (mk_env->d_thickness >> 1)), sw, w, 0);
	if (segs & 0x08)		/* D */
		draw_hseg(p + sw * (h - mk_env->d_thickness), sw, w, 2);
	
	if (segs & 0x20)		/* F */
		draw_vseg(p, sw, hh + 1, 0);
	if (segs & 0x02)		/* B */
		draw_vseg(p + w - mk_env->d_thickness, sw, hh + 1, 1);
	
	p += sw * hh;
	
	if (segs & 0x10)		/* E */
		draw_vseg(p, sw, hh + 1, 0);
	if (segs & 0x04)		/* C */
		draw_vseg(p + w - mk_env->d_thickness, sw, hh + 1, 1);
}

/* Draw single digit on the display */
static void draw_digit(u32 pos, u8 n, u32 flags)
{
	u8 dm;
	u32 dx, dy;
	u32 *p, *pp;
	u8 *l;
	u32 i;
	u32 pix;

	dx = mk_env->disp_x + pos * mk_env->disp_dwx;
	dy = mk_env->disp_y + ((mk_env->disp_h - mk_env->disp_dh) >> 1);

	p = mk_env->vbuf + dy * mk_env->mk_width + dx;
	l = mk_env->vlines + dy;

	if (flags & 1)
	{
		pp = p;
		for (i = 0; i < mk_env->disp_dh; i++)
		{
			fillaread(pp, mk_env->disp_color, mk_env->disp_dw);
			pp += mk_env->mk_width;
			*l++ = 0xFF;
		}
		dm = digitmask[n & 0x0F];
		if (dm)
			draw_segs(p, mk_env->mk_width, mk_env->disp_dw, mk_env->disp_dh, dm);
	}
	
	if (flags & 2)
	{
		p += mk_env->mk_width * (mk_env->disp_dh - mk_env->d_thickness) + mk_env->disp_dw + mk_env->d_thickness;
		l = mk_env->vlines + dy + (mk_env->disp_dh - mk_env->d_thickness);
		pix = (flags & 4) ? mk_env->digit_color : mk_env->disp_color;
		for (i = 0; i < mk_env->d_thickness; i++)
		{
			fillaread(p, pix, mk_env->d_thickness);
			p += mk_env->mk_width;
			*l++ = 0xFF;
		}
	}
}

/* Draw blank display with botder */
void calc_display_draw(void)
{
	u32 dx, dy, dw, dh;
	u32 b, w;
	u32 *p, *q;
	u8 *l;
	u32 i;
	u32 bc = mk_env->disp_frame;
	
	dx = mk_env->disp_x;
	dy = mk_env->disp_y;
	dw = mk_env->disp_w;
	dh = mk_env->disp_h;
	b = (dx < dy) ? dx >> 1 : dy >> 1;

	q = mk_env->vbuf + (dy - b) * mk_env->mk_width + (dx - b);
	l = mk_env->vlines + dy;
	w = dw + (b << 1);

	for (i = 0; i < b; i++)
	{
		fillaread(q, bc, w);
		q += mk_env->mk_width;
		*l++ = 0xFF;
	}

	for (i = 0; i < dh; i++)
	{
		fillaread(q, bc, b);
		fillaread(q + b + dw, bc, b);
		q += mk_env->mk_width;
		*l++ = 0xFF;
	}

	for (i = 0; i < b; i++)
	{
		fillaread(q, bc, w);
		q += mk_env->mk_width;
		*l++ = 0xFF;
	}

	p = mk_env->vbuf + dy * mk_env->mk_width + dx;
	l = mk_env->vlines + dy;
	for (i = 0; i < dh; i++)
	{
		fillaread(p, mk_env->disp_color, dw);
		p += mk_env->mk_width;
		*l++ = 0xFF;
	}
}

/* Synchronize graphical display with temporary display buffer */
void calc_display_sync(void)
{
	u32 i;
	u32 flags;
	u16 dpm, dpv;

	if (!(mk_env->flags & MK_POWER_ON))
	{
		mk_env->new_decimal_points = 0;
		fillareab(mk_env->new_sbuf, 0x0F, 12);
	}

	if (!(mk_env->flags & MK_SBUF_WRITTEN))
		return;

	mk_env->flags &= ~MK_SBUF_WRITTEN;
	dpm = 1;
	for (i = 0; i < 12; i++)
	{
		dpv = mk_env->new_decimal_points & dpm;
		if (dpv != (mk_env->cur_decimal_points & dpm))
			flags = 2 | (dpv ? 4 : 0);
		else
			flags = 0;
		if (mk_env->new_sbuf[i] != mk_env->cur_sbuf[i])
			flags |= 1;
		if (flags)
		{
			mk_env->cur_sbuf[i] = mk_env->new_sbuf[i];
			draw_digit(i, mk_env->cur_sbuf[i], flags);
		}
		dpm <<= 1;
	}
	mk_env->cur_decimal_points = mk_env->new_decimal_points;
}

/* Synchronize emu145 display buffer with temporary display buffer */
void emu145_sync_display(u8 *buf)
{
	mk_env->new_decimal_points = 0;
	if (buf)
	{
		u8 tbuf[12];
		u8 c;
		u8 *p;
		u32 i;

		p = tbuf;
		for (i = 0; i < 9; i++)
		{
			c = buf[8 - i];
			*p++ = c & 0x0F;
			if (c & 0x80)
				mk_env->new_decimal_points |= (1 << i);
		}
		for (i = 0; i < 3; i++)
		{
			c = buf[11 - i];
			*p++ = c & 0x0F;
			if (c & 0x80)
				mk_env->new_decimal_points |= (1 << (i + 9));
		}
		copyareab(mk_env->new_sbuf, tbuf, 12);
	}
//	else
//		fillareab(mk_env->new_sbuf, 0x0F, 12);
	mk_env->flags |= MK_SBUF_WRITTEN;
}
