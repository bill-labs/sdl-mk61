#include "microlib/microlib.h"
#include "cmem.h"

#include "console.h"

static u8 mem1[128];
static u8 mem2[128];

void cmem_reset(u32 n)
{
    u8 *ma = n ? mem2 : mem1;
	fillareab(ma, 0, 128);
}

u8 cmem_tick(u32 n, u8 rm)
{
	u8 *ma = n ? mem2 : mem1;
    u8 i = (ma[0] >> 1) + 1;
	u8 ret;

	if (ma[0] & 1)
	{
		ret = ma[i] >> 4;
		ma[i] = (ma[i] & 0x0F) | (rm << 4);	
	}
	else
	{
		ret = ma[i] & 0x0F;
		ma[i] = (ma[i] & 0xF0) | (rm & 0x0F);	
	}
	
	// deprintf("%X -> mem %d -> %X\n", rm, ma[0], ret);

	ma[0]++;
	if (ma[0] == 252)
		ma[0] = 0;
	return ret;
}

void memdump(void)
{
	u32 i;
	for (i = 1; i < 126; i++)
	{
		deprintf("%02X", mem1[i]);
	}
	deprintf(":");
	for (i = 1; i < 126; i++)
	{
		deprintf("%02X", mem2[i]);
	}
	deprintf("\n");
}
