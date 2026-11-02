#ifndef __EMU145_H
#define __EMU145_H

#include "cmcu13.h"
#include "cmem.h"

typedef struct _emu145
{
	cmcu_t *ik1302;
	cmcu_t *ik1303;
	cmcu_t *ik1306;
	u8 display[12];
	u8 sync;
    u32 dcycle;
    u32 btnpressed;
    s32 ustep;
} emu145_t;

#define ASW_RAD 9
#define ASW_DEG 10
#define ASW_GRD 11

#ifdef __cplusplus
extern "C" {
#endif

emu145_t *emu145_create(void);
void emu145_destroy(emu145_t *em);
void emu145_keypad(emu145_t *em, u32 key);
void emu145_run(emu145_t *em);
void emu145_sync_display(u8 *buf);
void emu145_reset(emu145_t *em);

#ifdef __cplusplus
}
#endif

#endif
