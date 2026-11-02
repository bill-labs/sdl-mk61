#ifndef __MKENV_H
#define __MKENV_H

#include "microlib/image.h"
#include "emu145.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _buttonrow
{
	u16		x_pos;
	u16		y_pos;
	u16		x_anchor;
	u16		y_anchor;
	image_t	image;
} buttonrow_t;

typedef struct _angleswitch
{
	u16		x_pos;
	u16		y_pos;
	u16		button_x_min;
	u16		button_x_max;
	u16		button_y;
	u16		button_pos;
	u32		button_off;
	image_t	image;
} angleswitch_t;

typedef struct _cgimage
{
	u16		x_pos;
	u16		y_pos;
	image_t	image;
} cgimage_t;

typedef struct _mkenv
{
	u16				mk_width;
	u16				mk_height;
	u16				mk_uwidth;
	u16				mk_uheight;
	u16				mk_x;
	u16				mk_y;

	buttonrow_t		*buttonrows[6];
	cgimage_t		*powerbutton;
	cgimage_t		*powerbutton_dim;
	cgimage_t		*namelabel;
	angleswitch_t	*angleswitch;
	u64				powerbutton_ts;
	u32				button_width;
	u32				button_height;
	u32				button_step;

	u32				disp_x;
	u32				disp_y;
	u32				disp_w;
	u32				disp_h;
	u32				disp_dw;
	u32				disp_dwx;
	u32				disp_dh;
	u32				disp_frame;
	u32				disp_color;
	u32				digit_color;

	u32				bgcolor;

	u32				*vbuf;
	u8				*vlines;
	
	u32				d_thickness;

	u32				flags;
	emu145_t		*emu145;
	u32				img_inited;
	s32				pressed_row;	/* -1 if none */
	u32				pressed_col;
	u32				pressed_active;

	u8				cur_sbuf[12];
	u8				new_sbuf[12];
	u16				cur_decimal_points;	
	u16				new_decimal_points;
	u32				tapped;
} mkenv_t;

extern mkenv_t *mk_env;

#define MK_POWER_ON				0x00000001
#define MK_SBUF_WRITTEN			0x00000002
#define MK_POINTERTAP_ON		0x00000004
#define MK_ANGLESWITCH_UPDATED	0x00000008
#define MK_ANGLESWITCH_MOVING	0x00000010
#define MK_POWERBUTTON_PRESSED	0x00000020
#define MK_POWERBUTTON_RELEASED	0x00000040

u32 mkenv_create(u32 width, u32 height, u32 *vbuf, u8 *vlines);
void mkenv_destroy(void);

u32 calc_button_event(u32 *px, u32 *py);
void calc_powerbutton_event(s32 x, s32 y, u32 on);
void calc_angleswitch_event(s32 x, s32 y, u32 on);
void calc_draw_buttonrow(u32 row);
void calc_draw_powerbutton(void);
void calc_draw_powerbutton_dim(void);
void calc_draw_namelabel(void);
void calc_draw_angleswitch(void);
void calc_draw_pressed(void);
void calc_invert_pressed(void);
void calc_dim_pressed(void);
void calc_dim_powerbutton(void);

u32 calc_graphics_init(void);
void calc_graphics_release(void);

void calc_display_draw(void);
void calc_display_sync(void);

#ifdef __cplusplus
}
#endif

#endif
