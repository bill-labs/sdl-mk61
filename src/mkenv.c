#include "microlib/microlib.h"
#include "microlib/heaps.h"
#include "mkenv.h"

#include "console.h"

mkenv_t *mk_env;

void mkenv_destroy(void)
{
	if (!mk_env)
		return;
	
	if (mk_env->emu145)
	{
		emu145_destroy(mk_env->emu145);
		mk_env->emu145 = NULL;
	}
	calc_graphics_release();
	mm_free(mk_env);
	mk_env = NULL;
}

void fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
u32 mkenv_create(u32 width, u32 height, u32 *vbuf, u8 *vlines)
{
	mkenv_t *env;

	if (!width || !height || !vbuf || !vlines)
		return 0;

	env = mm_malloc(sizeof(mkenv_t));
	if (!env)
		return 0;
	fillareab(env, 0, sizeof(mkenv_t));

	env->mk_width = width;
	env->mk_height = height;
	env->vbuf = vbuf;
	env->vlines = vlines;
	env->bgcolor = 0xFF303030;
	mk_env = env;

	fill_rect(0, 0, env->mk_width, env->mk_height, env->bgcolor);

	mk_env->disp_color = 0xFF6D6C66;
	mk_env->digit_color = 0xFF000000;
	mk_env->disp_frame = 0xFF808080;
	mk_env->pressed_row = -1;
	mk_env->pressed_col = 0;
	mk_env->pressed_active = 0;

	if (!calc_graphics_init())
		goto fail;

	mk_env->emu145 = emu145_create();
	if (!mk_env->emu145)
		goto fail;

	mk_env->flags |= MK_SBUF_WRITTEN | MK_ANGLESWITCH_UPDATED;
	fillareab(mk_env->new_sbuf, 0x0F, 12);
	mk_env->cur_decimal_points = 0x0FFF;

	calc_display_draw();
	return 1;

fail:
	mkenv_destroy();
	return 0;
}
