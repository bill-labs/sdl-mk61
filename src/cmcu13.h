/*
	cmcu13.h
	m145ik13

	Created by Felix Lazarev on 12/30/11.
	Copyright 2011 Felix Lazarev. All rights reserved.
*/

#ifndef __CMCU13_H
#define __CMCU13_H

#define UCMD_A_R		0x00000001
#define UCMD_A_M		0x00000002
#define UCMD_A_ST		0x00000004
#define UCMD_A_NR		0x00000008
#define UCMD_A_10NL		0x00000010
#define UCMD_A_S		0x00000020
#define UCMD_A_4		0x00000040

#define UCMD_B_S		0x00000080
#define UCMD_B_NS		0x00000100
#define UCMD_B_S1		0x00000200
#define UCMD_B_6		0x00000400
#define UCMD_B_1		0x00000800

#define UCMD_G_L		0x00001000
#define UCMD_G_NL		0x00002000
#define UCMD_G_NT		0x00004000

#define UCMD_R0_0		0x00008000
#define UCMD_R0_1		0x00010000
#define UCMD_R0_2		0x00020000

#define UCMD_R0_VAL(X)	(((X) >> 15) & 7)

#define UCMD_R_1		0x00040000
#define UCMD_R_2		0x00080000

#define UCMD_M			0x00100000
#define UCMD_L			0x00200000

#define UCMD_S_0		0x00400000
#define UCMD_S_1		0x00800000

#define UCMD_S_VAL(X)	(((X) >> 22) & 3)

#define UCMD_S1_0		0x01000000
#define UCMD_S1_1		0x02000000

#define UCMD_S1_VAL(X)	(((X) >> 24) & 3)

#define UCMD_ST_0		0x04000000
#define UCMD_ST_1		0x08000000

#define UCMD_ST_VAL(X)	(((X) >> 26) & 3)

typedef struct _cmcu
{
    u32		*_ucrom;            // ucommand rom
    u8		*_asprom;   		// synchro sequence rom
    u32		*_cmdrom;			// macrocommand rom   -- format (asp0)|(asp1<<8)|(asp2<<16)|(modflag<<24)
    u32		tic;
    u8		_rm[21];
    u8		_rr[21];
    u8		_rst[21];
    u8		_rs;
    u8		_rs1;

    u8		_dispout;
    u8		rl;
    u8		rt;
    u32 	_sigma; 			//	so we can calculate normally
    u8 		carry;
    u32		u_command;
    u32		command;
    u8		cur_ucmd;
    u32		dcount;
	u32		ecount;
    u8		cptr;
} cmcu_t;

#define ICOUNT(cmcu)	(cmcu->tic)

cmcu_t *cmcu_create(void);
void cmcu_destroy(cmcu_t *cmcu);
void cmcu_reset(cmcu_t *cmcu);
void cmcu_pretick(cmcu_t *cmcu, u8 rin);

/* execute 1 tick of cpu, 
	parameters:	rin - input register; 
				k1 -- input pin K1 or H!!!
                k2 -- input pin K2
                *dcycle  -- current D cycle, output
                *syncout -- synchronization output for slaves
                *segment -- bit to display
                *point   -- decimal point
	return value = output register
*/
u8 cmcu_tick(cmcu_t *cmcu, u8 rin, u8 k, u32 *dcycle, u8 *syncout, u8 *segment);
u8 cmcu_strobe(cmcu_t *cmcu);

#endif
