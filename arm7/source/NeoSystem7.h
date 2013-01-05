#ifndef _NEO_SYSTEM7_H
#define _NEO_SYSTEM7_H

#include "NeoSystemCommon.h"
#include "NeoCpuZ80.h"

#define NEO_Z80 0
//#define NEO_ADPCM (NEO_Z80 + 112)
//#define NEO_ADPCMB (NEO_ADPCM + 12 * 7) //7 channels, 12 bytes per channel
//#define NEO_ADPCM_ACTIVE (NEO_ADPCMB + 12) //1 channel, 12 bytes
#define NEO_Z80MEMTABLE (NEO_Z80 + 112)
#define NEO_Z80RAM (NEO_Z80MEMTABLE + MEMBLOCK_COUNT * 4)
#define NEO_CONTEXTEND7 (NEO_Z80RAM + 2048)

#define Z80_CLOCK_DIVIDE 6

#define Z80_CLOCK (MAIN_CLOCK / Z80_CLOCK_DIVIDE)

#define Z80_CLOCKS_PER_SCANLINE (PIXELS_PER_SCANLINE * PIXEL_CLOCK_DIVIDE / Z80_CLOCK_DIVIDE)
#define Z80_CLOCKS_PER_FRAME (Z80_CLOCKS_PER_SCANLINE * SCANLINES_PER_FRAME)
#define Z80_CLOCKS_PER_VBLANK (Z80_CLOCKS_PER_SCANLINE * SCANLINES_PER_VBLANK)
#define Z80_CLOCKS_PER_TOP (Z80_CLOCKS_PER_SCANLINE * SCANLINES_TOP)
#define Z80_CLOCKS_PER_BOTTOM (Z80_CLOCKS_PER_SCANLINE * SCANLINES_BOTTOM)
#define Z80_CLOCKS_PER_ACTIVE (Z80_CLOCKS_PER_SCANLINE * SCANLINES_ACTIVE)

#define Z80_TIMESLICE_PER_FRAME 200
#define Z80_CLOCKS_PER_TIMESLICE (Z80_CLOCKS_PER_FRAME / Z80_TIMESLICE_PER_FRAME)

#ifndef NEO_IN_ASM

typedef struct _TNeoContext7 {
	TDrZ80Context z80;
	//audio stream
	//TNeoADPCMStream adpcm[7];
	//TNeoADPCMBStream adpcmb;
	//u32 adpcmActive;

	u8* z80MemTable[MEMBLOCK_COUNT];
	u8 z80Ram[2048];
} TNeoContext7;

void neoSystem7Init();
void neoSystem7Reset();
void neoSystem7Execute();

void systemPanic();

#ifndef NEO_SHIPPING
#define ASSERT(x) (void)((x) || (systemPanic(), 0))
#else
#define ASSERT(x) ((void)0)
#endif

#define KB (1<<10)
#define MB (1<<20)

register TNeoContext7* g_neo7 asm("r5");

#endif //NEO_IN_ASM

#endif
