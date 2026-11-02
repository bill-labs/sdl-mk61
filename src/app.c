#include "microlib/microlib.h"
#include "microlib/app.h"
#include "microlib/video.h"
#include "microlib/input.h"
#include "microlib/clock.h"
#include "mkenv.h"

#include "console.h"

/* 
	* Button size 361x249
	* Button step 470
	* row0	2306x455	x=23,y=244	bottom=87	right = 42
	* row1	2321x373	x=0,y=249	bottom=0	right = 80
	* row2	2355x381	x=19,y=257	bottom=0	right = 0
	* row3	2241x370	x=0,y=246	bottom=0	right = 0
	* row4	2241x349	x=0,y=225	bottom=0	right = 0
	* row5	2241x349	x=0,y=225	bottom=0	right = 0
*/

/* buttons01: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 */
/* buttons10: +, -, *, /, xch, ., /-/, ВП, Cx, B^ */
/* buttons11: С/П, БП, В/0, ПП, П, ШГ+, ИП, ШГ-, К, F */
static u16 keynumbers[] = {
	0x309, 0x305, 0x307, 0x302, 0x300,
	0x308, 0x306, 0x304, 0x301, 0x303,
	0x107, 0x108, 0x109, 0x201, 0x203,
	0x104, 0x105, 0x106, 0x200, 0x202,
	0x101, 0x102, 0x103, 0x204, 0x209,
	0x100, 0x205, 0x206, 0x207, 0x208
};

static void screen_tap(u32 x, u32 y, u32 on)
{
	u32 buttons_y = 150;

	if (on)
		mk_env->flags |= MK_POINTERTAP_ON;
	else
		mk_env->flags &= ~MK_POINTERTAP_ON;
	
	calc_powerbutton_event(x, y, on);
	calc_angleswitch_event(x, y, on);

	if (!on)
	{
		if (!mk_env->tapped)
			return;
		emu145_keypad(mk_env->emu145, 0);
		mk_env->tapped = 0;
		return;
	}

	if (y < buttons_y)
		return;
	if (!calc_button_event(&x, &y))
		return;

	mk_env->tapped = keynumbers[y + (y << 2) + x];
	emu145_keypad(mk_env->emu145, mk_env->tapped);
}

static void check_for_input(void)
{
	key_event_t kev;
	pointer_event_t pev;
	
	while (!mi_get_pointer_event((event_t *)&pev))
	{
		if (pev.event_id == POINTER_TAP)
			screen_tap(pev.x, pev.y, 1);
		if (pev.event_id == POINTER_UNTAP)
			screen_tap(pev.x, pev.y, 0);
		if (pev.event_id == POINTER_MOVEABS)
			calc_angleswitch_event(pev.x, pev.y, mk_env->flags & MK_POINTERTAP_ON);
	}
	
	while (!mi_get_key_event((event_t *)&kev))
	{
	}
}

void calc_draw_angleswitch(void);
static void sync_ui(void)
{
	if (mk_env->flags & MK_ANGLESWITCH_UPDATED)
	{
		calc_draw_angleswitch();
		mk_env->flags &= ~MK_ANGLESWITCH_UPDATED;
	}
}

s32 ml_app_init_env(ml_app_sysenv_t *env)
{
	u32 t;
	
	if (env->flags & ML_ENV_WINDOWED_DESKTOP)
	{
		// XXX use desktop dimensions
		env->app_window_width = 320;
		env->app_window_height = 480;
	}
	else if (env->flags & ML_ENV_SCREEN_CAN_ROTATE)
	{
		if (env->app_window_width > env->app_window_height)
		{
			/* Use portrait orientation */
			t = env->app_window_width;
			env->app_window_width = env->app_window_height;
			env->app_window_height = t;
		}
	}

	if (env->flags & ML_ENV_DESKTOP_HAS_CURSOR)
		env->flags |= ML_ENV_DESKTOP_NOT_HIDE_CURSOR;

	return 0;
}

s32 ml_app_startup(ml_app_info_t *ai)
{
	if (!mkenv_create())
		return 1;

	ai->app_run_period = 400000;
	return 0;	
}

s32 ml_app_cleanup(ml_app_info_t *ai)
{
	mkenv_destroy();
	return 0;
}

s32 ml_app_run(ml_app_info_t *ai)
{
	u32 i;

	sync_ui();
	calc_display_sync();

	/* Run for 1/25 s */
	for (i = 0; i < 96; i++)
	{
		check_for_input();
		emu145_run(mk_env->emu145);
	}

	return 0;
}
