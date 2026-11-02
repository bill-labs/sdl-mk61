#include "microlib/microlib.h"
#include "microlib/clock.h"
#include "mkenv.h"
#include "emu145.h"

#include <SDL/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static SDL_Surface *screen = NULL;
static u32 *framebuf = NULL;
static u8 *vlines = NULL;
static int running = 1;
static int screen_bpp = 0;
static int screen_pitch = 0;
static int win_w = 320;
static int win_h = 480;
static SDLKey kb_sym = SDLK_UNKNOWN;

static u16 keynumbers[] = {
	0x309, 0x305, 0x307, 0x302, 0x300,
	0x308, 0x306, 0x304, 0x301, 0x303,
	0x107, 0x108, 0x109, 0x201, 0x203,
	0x104, 0x105, 0x106, 0x200, 0x202,
	0x101, 0x102, 0x103, 0x204, 0x209,
	0x100, 0x205, 0x206, 0x207, 0x208};

static u16 keynumbers_digit[] = {
	0x303, /* 0 */
	0x101, /* 1 */
	0x102, /* 2 */
	0x103, /* 3 */
	0x104, /* 4 */
	0x105, /* 5 */
	0x106, /* 6 */
	0x107, /* 7 */
	0x108, /* 8 */
	0x109  /* 9 */
};

static int key_to_code(SDLKey sym, u16 *code)
{
	if (sym >= SDLK_0 && sym <= SDLK_9)
	{
		*code = keynumbers_digit[(u32)(sym - SDLK_0)];
		return 1;
	}
	if (sym >= SDLK_KP0 && sym <= SDLK_KP9)
	{
		*code = keynumbers_digit[(u32)(sym - SDLK_KP0)];
		return 1;
	}
	if (sym == SDLK_ESCAPE)
	{
		*code = keynumbers[29]; /* Cx */
		return 1;
	}
	if (sym == SDLK_f)
	{
		*code = keynumbers[0]; /* F */
		return 1;
	}
	if (sym == SDLK_k)
	{
		*code = keynumbers[5]; /* K */
		return 1;
	}
	if (sym == SDLK_UP)
	{
		*code = keynumbers[24]; /* B^ */
		return 1;
	}
	if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER)
	{
		*code = keynumbers[24]; /* B^ */
		return 1;
	}
	if (sym == SDLK_LEFT)
	{
		*code = keynumbers[2]; /* ШГ- */
		return 1;
	}
	if (sym == SDLK_RIGHT)
	{
		*code = keynumbers[1]; /* ШГ+ */
		return 1;
	}
	if (sym == SDLK_PLUS || sym == SDLK_EQUALS || sym == SDLK_KP_PLUS)
	{
		*code = keynumbers[18]; /* + */
		return 1;
	}
	if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS)
	{
		*code = keynumbers[13]; /* - */
		return 1;
	}
	if (sym == SDLK_ASTERISK || sym == SDLK_KP_MULTIPLY)
	{
		*code = keynumbers[19]; /* * */
		return 1;
	}
	if (sym == SDLK_SLASH || sym == SDLK_KP_DIVIDE)
	{
		*code = keynumbers[14]; /* / */
		return 1;
	}
	return 0;
}

static void screen_tap(u32 x, u32 y, u32 on)
{
	u32 rx = x;
	u32 ry = y;
	u32 buttons_y = 150;

	if (on)
		mk_env->flags |= MK_POINTERTAP_ON;
	else
		mk_env->flags &= ~MK_POINTERTAP_ON;

	calc_powerbutton_event((s32)rx, (s32)ry, on);
	calc_angleswitch_event((s32)rx, (s32)ry, on);

	if (!on)
	{
		if (!mk_env->tapped)
			return;
		emu145_keypad(mk_env->emu145, 0);
		mk_env->tapped = 0;
		if (mk_env->pressed_active && mk_env->pressed_row >= 0)
			calc_draw_buttonrow((u32)mk_env->pressed_row);
		mk_env->pressed_active = 0;
		mk_env->pressed_row = -1;
		return;
	}

	if (ry < buttons_y)
		return;
	u32 bx = rx;
	u32 by = ry;
	if (!calc_button_event(&bx, &by))
		return;

	mk_env->tapped = keynumbers[by + (by << 2) + bx];
	emu145_keypad(mk_env->emu145, mk_env->tapped);
	mk_env->pressed_row = 5 - by;
	mk_env->pressed_col = bx;
	mk_env->pressed_active = 1;
}

static void handle_events(void)
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_QUIT:
			running = 0;
			break;
	case SDL_MOUSEBUTTONDOWN:
		screen_tap((u32)ev.button.x, (u32)ev.button.y, 1);
		break;
	case SDL_MOUSEBUTTONUP:
		screen_tap((u32)ev.button.x, (u32)ev.button.y, 0);
		if (mk_env->pressed_active && mk_env->pressed_row >= 0)
			calc_draw_buttonrow((u32)mk_env->pressed_row);
		mk_env->pressed_active = 0;
		mk_env->pressed_row = -1;
		break;
		case SDL_MOUSEMOTION:
			calc_angleswitch_event((s32)ev.motion.x, (s32)ev.motion.y, mk_env->flags & MK_POINTERTAP_ON);
			break;
	case SDL_KEYDOWN:
		{
			u16 keycode = 0;
			if (key_to_code(ev.key.keysym.sym, &keycode))
			{
				if (kb_sym == SDLK_UNKNOWN)
				{
					kb_sym = ev.key.keysym.sym;
					mk_env->tapped = keycode;
					emu145_keypad(mk_env->emu145, mk_env->tapped);
					mk_env->pressed_active = 0;
				}
			}
		}
		break;
	case SDL_KEYUP:
		if (kb_sym != SDLK_UNKNOWN && ev.key.keysym.sym == kb_sym)
		{
			emu145_keypad(mk_env->emu145, 0);
			mk_env->tapped = 0;
			kb_sym = SDLK_UNKNOWN;
			if (mk_env->pressed_active && mk_env->pressed_row >= 0)
				calc_draw_buttonrow((u32)mk_env->pressed_row);
			mk_env->pressed_active = 0;
			mk_env->pressed_row = -1;
		}
		break;
		default:
			break;
		}
	}
}

static void sync_ui(void)
{
	if (mk_env->flags & MK_ANGLESWITCH_UPDATED)
	{
		calc_draw_angleswitch();
		mk_env->flags &= ~MK_ANGLESWITCH_UPDATED;
	}
	for (u32 row = 0; row < 6; row++)
		calc_draw_buttonrow(row);
	if (mk_env->flags & MK_POWERBUTTON_PRESSED)
		calc_draw_powerbutton_dim();
	else
		calc_draw_powerbutton();
	calc_draw_namelabel();
	if (mk_env->pressed_active)
		calc_dim_pressed();
	if (mk_env->flags & MK_POWERBUTTON_RELEASED)
		mk_env->flags &= ~(MK_POWERBUTTON_PRESSED | MK_POWERBUTTON_RELEASED);
}

static void blit_to_screen(void)
{
	if (!screen || !framebuf)
		return;

	if (SDL_LockSurface(screen) != 0)
		return;

	u8 *dst = (u8 *)screen->pixels;
	int pitch_dst = screen_pitch;

	int copy = win_w * 4;
	for (u32 y = 0; y < (u32)win_h; y++)
		memcpy(dst + y * pitch_dst, ((u8 *)framebuf) + y * copy, copy);

	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

static int init_video(int width, int height)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		return 0;

	screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
	if (!screen)
		return 0;
	if (screen->w != width || screen->h != height)
	{
		SDL_FreeSurface(screen);
		screen = NULL;
		SDL_Quit();
		return 0;
	}
	SDL_WM_SetCaption("MK-61", "MK-61");

	screen_bpp = screen->format->BytesPerPixel;
	screen_pitch = screen->pitch;
	if (screen_bpp != 4)
	{
		SDL_FreeSurface(screen);
		screen = NULL;
		SDL_Quit();
		return 0;
	}

	framebuf = (u32 *)malloc((size_t)width * (size_t)height * sizeof(u32));
	vlines = (u8 *)malloc((size_t)height);
	if (!framebuf || !vlines)
	{
		if (framebuf)
			free(framebuf);
		if (vlines)
			free(vlines);
		SDL_FreeSurface(screen);
		screen = NULL;
		SDL_Quit();
		return 0;
	}
	fillareab(vlines, 0, (size_t)height);
	return 1;
}

static void shutdown_video(void)
{
	if (screen)
		SDL_FreeSurface(screen);
	screen = NULL;
	if (vlines)
		free(vlines);
	vlines = NULL;
	if (framebuf)
		free(framebuf);
	framebuf = NULL;
	SDL_Quit();
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	win_w = 320;
	win_h = 480;

	const int width = win_w;
	const int height = win_h;
	SDL_putenv("SDL_DISABLE_MMX=1");
	SDL_putenv("SDL_DISABLE_SSE=1");

	if (!init_video(width, height))
	{
		fprintf(stderr, "SDL init failed\n");
		return 1;
	}

	if (!mkenv_create((u32)win_w, (u32)win_h, framebuf, vlines))
	{
		fprintf(stderr, "Failed to init environment\n");
		shutdown_video();
		return 1;
	}

	/* Initial full draw */
	calc_display_draw();
	for (u32 row = 0; row < 6; row++)
		calc_draw_buttonrow(row);
	calc_draw_powerbutton();
	calc_draw_namelabel();
	calc_draw_angleswitch();
	calc_display_sync();

	while (running)
	{
		handle_events();
		sync_ui();

		for (int i = 0; i < 96; i++)
			emu145_run(mk_env->emu145);

		calc_display_sync();
		blit_to_screen();
		SDL_Delay(10);
	}

	mkenv_destroy();
	shutdown_video();
	return 0;
}
