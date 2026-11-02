/*
	cmcu13.c
	m145ik13

	Created by Felix Lazarev on 12/30/11.
	Copyright 2011 Felix Lazarev. All rights reserved.
*/

#include "microlib/microlib.h"
#include "microlib/heaps.h"

#include "cmcu13.h"

#include "console.h"

const u8 jrom[42]={
    0, 1, 2, 3, 4, 5, 3, 4, 5, 
    3, 4, 5, 3, 4, 5, 3, 4, 5, 
    3, 4, 5, 3, 4, 5, 6, 7, 8,
    0, 1, 2, 3, 4, 5, 6, 7, 8,
    0, 1, 2, 3, 4, 5
};

cmcu_t *cmcu_create(void)
{
	cmcu_t *cmcu;
 
	cmcu = (cmcu_t *)mm_malloc(sizeof(cmcu_t));
	if (!cmcu)
		return NULL;

	fillareab(cmcu, 0, sizeof(cmcu_t));
	return cmcu;
}

void cmcu_destroy(cmcu_t *cmcu)
{
	mm_free(cmcu);
}

void cmcu_reset(cmcu_t *cmcu)
{
	cmcu->tic = 0;
	fillareab(cmcu->_rm, 0, 21);
	fillareab(cmcu->_rr, 0, 21);
	fillareab(cmcu->_rst, 0, 21);
	cmcu->_rs = 0;
	cmcu->_rs1 = 0;
	cmcu->_dispout = 0;
    cmcu->rl = 0;
    cmcu->rt = 0;
    cmcu->_sigma = 0;
    cmcu->carry = 0;
	cmcu->u_command = 0;
	cmcu->cur_ucmd = 0;
    cmcu->dcount = 0;
    cmcu->ecount = 0;
    cmcu->cptr = 0;
    cmcu->command = cmcu->_cmdrom[cmcu->cptr];
}

static void set_m4bit(cmcu_t *cmcu, u32 idx, u8 v)
{
	u32 tic = cmcu->tic + idx;
	u8 i;

	if (tic >= 42)
		tic -= 42;

	i = (u8)(tic >> 1);
	if (tic & 1)
		cmcu->_rm[i] = (cmcu->_rm[i] & 0x0F) | (v << 4);
	else
		cmcu->_rm[i] = (cmcu->_rm[i] & 0xF0) | v;
}

void cmcu_pretick(cmcu_t *cmcu, u8 rin)
{
	u32 tic;
	u8 i;

	if (!cmcu->tic)
		tic = 41;
	else
		tic = cmcu->tic - 1;

	i = (u8)(tic >> 1);
	if (tic & 1)
		cmcu->_rm[i] = (cmcu->_rm[i] & 0x0F) | (rin << 4);
	else
		cmcu->_rm[i] = (cmcu->_rm[i] & 0xF0) | rin;
}

static u32 get_r4bit(cmcu_t *cmcu, u32 idx)
{
	u32 tic = cmcu->tic + idx;
	u8 i, shift;

	if (tic >= 42)
		tic -= 42;

	i = (u8)(tic >> 1);
	shift = (tic & 1) << 2;

	return (cmcu->_rr[i] >> shift) & 0x0F;
}

static void set_r4bit(cmcu_t *cmcu, u32 idx, u8 v)
{
	u32 tic = cmcu->tic + idx;
	u8 i;

	if (tic >= 42)
		tic -= 42;

	i = (u8)(tic >> 1);
	if (tic & 1)
		cmcu->_rr[i] = (cmcu->_rr[i] & 0x0F) | (v << 4);
	else
		cmcu->_rr[i] = (cmcu->_rr[i] & 0xF0) | v;
}

static s32 cmcu_exec_ucmd(cmcu_t *cmcu, u8 rin, u8 k)
{
    s32 a;
    s32 b;
    s32 g;
    s32 newr0;
    s32 newm0;
	u32 t32, x, y, z;
	u8 ri, ri1, rshift;
	u32 sigmar;
    
	ri = (u8)(cmcu->tic >> 1);
	rshift = (cmcu->tic & 1) << 2;

	if (!((cmcu->command >> 18) & 0x3F))
		cmcu->rt = 0;
		
	if (!(k & 0x80) && (cmcu->u_command & UCMD_S1_1))
		cmcu->_rs1 |= k & 0x0F;

    a = 0;
    if (cmcu->u_command & UCMD_A_R)
		a |= (cmcu->_rr[ri] >> rshift) & 0x0F;

    if (cmcu->u_command & UCMD_A_M)
		a |= (cmcu->_rm[ri] >> rshift) & 0x0F;

    if (cmcu->u_command & UCMD_A_ST)
		a |= (cmcu->_rst[ri] >> rshift) & 0x0F;

    if (cmcu->u_command & UCMD_A_NR)
		a |= ((cmcu->_rr[ri] >> rshift) ^ 0x0F) & 0x0F;

    if (cmcu->u_command & UCMD_A_10NL)
    {
		if (!cmcu->rl)
			a |= 10;
	}

    if (cmcu->u_command & UCMD_A_S)
		a |= cmcu->_rs;

    if (cmcu->u_command & UCMD_A_4)
        a |= 4;

    b = 0;
    if (cmcu->u_command & UCMD_B_S)
        b |= cmcu->_rs;

    if (cmcu->u_command & UCMD_B_NS)
        b |= (cmcu->_rs ^ 0x0F) & 0x0F;

    if (cmcu->u_command & UCMD_B_S1)
        b |= cmcu->_rs1;

    if (cmcu->u_command & UCMD_B_6)
        b |= 6;

    if (cmcu->u_command & UCMD_B_1)
        b |= 1;
 
    if ((cmcu->command >> 18) & 0x3F)
	{
		if (!(k & 0x0F))
			cmcu->rt = 0;
	}
	else
	{
		// u16 dmask;
		if ((k & 0x80) && (k & 0x0F))
		{
			cmcu->_rs1 = k & 0x0F;
			cmcu->rt = 1;
		}
#if 0
		dmask = (1 << cmcu->dcount);
		if (cmcu->rl)
			cmcu->decimals |= dmask;
		else
			cmcu->decimals &= ~dmask;			
#endif
	}
     
	g = 0;
    if (cmcu->u_command & UCMD_G_L)
		g |= cmcu->rl & 1;
	if (cmcu->u_command & UCMD_G_NL)
		g |= ~cmcu->rl & 1;
	if (cmcu->u_command & UCMD_G_NT)
		g |= ~cmcu->rt & 1;
   
    cmcu->_sigma = a + b + g;
    cmcu->carry = (cmcu->_sigma >> 4) & 1;
    cmcu->_sigma &= 0x0F;
    
    if (!((ICOUNT(cmcu) < 36) && (cmcu->command & 0xFF000000)))		/* mod flag -- do not modify R!!! */
	{
		switch (UCMD_R0_VAL(cmcu->u_command))
		{
			case 0:
				newr0 = (cmcu->_rr[ri] >> rshift) & 0x0F;
				break;

			case 1:
				newr0 = get_r4bit(cmcu, 3);
				break;

			case 2:
				newr0 = cmcu->_sigma;
				break;

			case 3:
				newr0 = cmcu->_rs;
				break;

			case 4:
				newr0 = (cmcu->_rr[ri] >> rshift) & 0x0F;
				newr0 |= cmcu->_rs;
				newr0 |= cmcu->_sigma;
				break;

			case 5:
				newr0 = cmcu->_rs;
				newr0 |= cmcu->_sigma;
				break;

			case 6:
				newr0 = (cmcu->_rr[ri] >> rshift) & 0x0F;
				newr0 |= cmcu->_rs;
				break;

			case 7:
				newr0 = (cmcu->_rr[ri] >> rshift) & 0x0F;
				newr0 |= cmcu->_sigma;
				break;
		}
		set_r4bit(cmcu, 0, newr0);
		
		if (cmcu->u_command & UCMD_R_1)
			sigmar = 1;
		else if (cmcu->u_command & UCMD_R_2)
			sigmar = 2;
		else
			sigmar = 0;

		if (sigmar)
			set_r4bit(cmcu, 42 - sigmar, cmcu->_sigma);		
	}

    if ((cmcu->dcount < 13) && (cmcu->ecount == 0))
		cmcu->_dispout = (cmcu->_rr[ri] >> rshift) & 0x0F;

    if (cmcu->u_command & UCMD_M)
		set_m4bit(cmcu, 0, cmcu->_rs);

    if (cmcu->u_command & UCMD_L)
		cmcu->rl = cmcu->carry;

    switch (UCMD_S_VAL(cmcu->u_command))
    {
        case 0:
            break;

        case 1:
			cmcu->_rs = cmcu->_rs1;
            break;

        case 2:
			cmcu->_rs = cmcu->_sigma;
            break;

        case 3:
			cmcu->_rs = cmcu->_rs1 | cmcu->_sigma;
            break;
    }

    switch (UCMD_S1_VAL(cmcu->u_command))
    {
        case 0:
        case 2:
            break;

        case 1:
			cmcu->_rs1 = cmcu->_sigma;
            break;

        case 3:
			cmcu->_rs1 |= cmcu->_sigma;	
            break;
    }

	if (UCMD_ST_VAL(cmcu->u_command))
	{
		t32 = cmcu->_rst[ri];
		ri1 = ri + 1;
		if (ri1 == 21)
			ri1 = 0;
		t32 |= (u32)cmcu->_rst[ri1] << 8;
		if (rshift)
			t32 = (t32 >> 4) | ((t32 << 12) & 0xF000);

		switch (UCMD_ST_VAL(cmcu->u_command))
		{
			case 1:
				t32 = (t32 & 0xF000) | ((t32 << 4) & 0x0FF0) | cmcu->_sigma;
				break;

			case 2:
				t32 = (t32 & 0xF000) | ((t32 >> 4) & 0x00FF) | ((t32 << 8) & 0x0F00);
				break;

			case 3:
				x = t32 & 0x0F;
				y = (t32 >> 4) & 0x0F;
				z = (t32 >> 8) & 0x0F;
				t32 = (t32 & 0xF000) | ((y | x) << 8) | ((x | z) << 4) | (cmcu->_sigma | y);
				break;
		}

		if (rshift)
			t32 = (t32 << 4) | ((t32 >> 12) & 0x000F);
		cmcu->_rst[ri] = t32 & 0xFF;
		cmcu->_rst[ri1] = t32 >> 8;	
	}

	if (rshift)
	{
		newm0 = cmcu->_rm[ri] >> 4;
		cmcu->_rm[ri] = (cmcu->_rm[ri] & 0x0F) | (rin << 4);
	}
	else
	{
		newm0 = cmcu->_rm[ri] & 0x0F;
		cmcu->_rm[ri] = (cmcu->_rm[ri] & 0xF0) | (rin & 0x0F);
	}

    return newm0;
}

u8 cmcu_tick(cmcu_t *cmcu, u8 rin, u8 k, u32 *dcycle, u8 *syncout, u8 *segment)
{
    u32 ret;
    u8 ucmd;
    u8 asp;
    u8 seg;

    if (cmcu->tic < 27)
		asp = cmcu->command & 0x7F;
    else if ((cmcu->tic >= 27) && (cmcu->tic < 36))
		asp = (cmcu->command >> 8) & 0x7F;
	else
	{
		asp = (cmcu->command >> 16) & 0xFF;
		/* special case */
		if (asp >= 0x20)
		{
			/* r0 points to i = 36 */
			/* we need to store ASP field to R1[D14:D13] */
			if (cmcu->tic == 36)
			{
				set_r4bit(cmcu, 1, asp & 0x0F);
				set_r4bit(cmcu, 4, (asp >> 4) & 0x0F);
			}
			asp = 0x5F;
		}
		else
			asp &= 0x3F;
	}

	ucmd = cmcu->_asprom[((asp << 3) + asp) + jrom[cmcu->tic]]; 
    ucmd &= 0x3F;		/* fool's proof */
    if (ucmd > 0x3B)
    {
        ucmd = (ucmd - 0x3C) * 2;
        ucmd += (!cmcu->rl) ? 1 : 0;
        ucmd += 0x3C;
    }
    cmcu->cur_ucmd = ucmd;
    cmcu->u_command = cmcu->_ucrom[ucmd];
    
	ret = cmcu_exec_ucmd(cmcu, rin, k);
  
	cmcu->tic++;
	if (cmcu->tic == 42)
	{
		cmcu->tic = 0;
		cmcu->cptr = (get_r4bit(cmcu, 39) << 4) | get_r4bit(cmcu, 36);
        cmcu->command = cmcu->_cmdrom[cmcu->cptr];
        cmcu->rt = 0;
	}

	cmcu->ecount++;
    if (cmcu->ecount >= 3)
    {
        cmcu->ecount = 0;
        cmcu->dcount++;
    }

    if (cmcu->dcount >= 14)
        cmcu->dcount = 0;

    /* only needed for master - 1302 */
    if ((dcycle) && (syncout))
    {
        *dcycle = (((cmcu->command & 0xFC0000) == 0) && (cmcu->ecount == 0)) ? (cmcu->dcount + 1) : 0;   /* return value is D strobe idx for keyboard/display scan */
        *syncout = ((cmcu->dcount == 13) && (cmcu->ecount == 2)) ? 1 : 0;
    }

    if (segment)
    {
		if ((cmcu->command & 0xFC0000) == 0)
		{
			seg = ((cmcu->ecount == 0)) ? cmcu->_dispout : 0;
			if (cmcu->rl)
				seg |= 0x80;
			*segment = seg;
		}
		else
			*segment = 0;
    }

    return ret & 0x0F;
}

u8 cmcu_strobe(cmcu_t *cmcu)
{
    return (((cmcu->command & 0xFC0000) == 0)) ? 1 : 0;   /* return value is D strobe idx for keyboard/display scan */
}
