#include "microlib/microlib.h"
#include "microlib/heaps.h"
#include "microlib/clock.h"
#include "mkenv.h"

#include "emu145.h"
#include "ucommands.h"
#include "synchro.h"
#include "mcommands.h"

#include "console.h"

void emu145_reset(emu145_t *em)
{
	cmcu_reset(em->ik1302);
	cmcu_reset(em->ik1303);
	cmcu_reset(em->ik1306);
	cmem_reset(0);
	cmem_reset(1);

	em->sync = 0;
	em->ustep = 0;
	em->btnpressed = 0;
}

emu145_t *emu145_create(void)
{
	emu145_t *em;
	
	em = mm_malloc(sizeof(emu145_t));
	if (!em)
		return NULL;

	em->ik1302 = cmcu_create();
	em->ik1303 = cmcu_create();
	em->ik1306 = cmcu_create();

	/* load memory */
	em->ik1302->_ucrom = (u32 *)ik1302_urom;
	em->ik1303->_ucrom = (u32 *)ik1303_urom;
	em->ik1306->_ucrom = (u32 *)ik1306_urom;

	em->ik1302->_asprom = (u8 *)ik1302_srom;
	em->ik1303->_asprom = (u8 *)ik1303_srom;
	em->ik1306->_asprom = (u8 *)ik1306_srom;


	em->ik1302->_cmdrom = ik1302_mrom;
	em->ik1303->_cmdrom = ik1303_mrom;
	em->ik1306->_cmdrom = ik1306_mrom;

	emu145_reset(em);
	return em;
}

void emu145_destroy(emu145_t *em)
{
	mm_free(em);
}

void emu145_keypad(emu145_t *em, u32 key)
{
	if(em->btnpressed)
		return; 					/* already processing one */    
    em->btnpressed = key;
}

u8 emu145_get_key(void);
void emu145_run(emu145_t *em)
{
	u32 aswp, asw;
	u8 seg;
	u32 cycle;
	u32 maxcycle;
	u8 chain;
	u8 k;

	if (!(mk_env->flags & MK_POWER_ON))
		return;

	if (mk_env->flags & MK_POWERBUTTON_PRESSED)
	{
		if ((mk_env->flags & MK_POWER_ON) && (mc_get_run_time() - mk_env->powerbutton_ts >= 3000000))
		{
			mk_env->flags &= ~(MK_POWER_ON | MK_POWERBUTTON_PRESSED);
			emu145_reset(mk_env->emu145);
			mk_env->flags |= MK_SBUF_WRITTEN;
		}
	}

	if (em->btnpressed)
		k = ((em->btnpressed >> 8) & 1) | (((em->btnpressed >> 9) & 1) << 3);
	else
		k = 0;

	maxcycle = 42;
	for (cycle = 0; cycle < maxcycle; cycle++)
	{
		
		if (cmcu_strobe(em->ik1302))
		{		
			if (em->ik1302->dcount == 12) /* D13 */
				k |= (1 << 3);
			if (em->btnpressed)
			{
				if (em->ik1302->dcount == (em->btnpressed & 0xFF) + 1)
				{
					k |= 0x80;
					em->btnpressed = 0;
				}
				else
					k &= 0x7F;
			}
		}
		else
			k = 0;

		chain = cmcu_tick(em->ik1302, 0, k, &em->dcycle, &em->sync, &seg);

		if (cmcu_strobe(em->ik1302))
		{
			if ((em->dcycle > 1) && (em->dcycle < 14))
			{
				em->display[em->dcycle - 2] = seg;
			}
			if (em->sync)
				emu145_sync_display(em->display);
		}
		else
		{
			if(em->sync)
				emu145_sync_display(NULL);
		}

#if 1
		aswp = mk_env->angleswitch->button_pos;
		if (aswp >= 66)
			asw = ASW_DEG;
		else if (aswp <= 33)
			asw = ASW_RAD;
		else
			asw = ASW_GRD;
		if (em->ik1303->dcount == asw)
			asw = 0x81;
		else
			asw = 1;

		chain = cmcu_tick(em->ik1303, chain, asw, NULL, NULL, NULL);
#endif

#if 1
		chain = cmcu_tick(em->ik1306, chain, 0, NULL, NULL, NULL);
#endif
		chain = cmem_tick(0, chain);
		chain = cmem_tick(1, chain);
		cmcu_pretick(em->ik1302, chain);
	}
}
