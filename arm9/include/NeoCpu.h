#ifndef _NEO_CPU_H
#define _NEO_CPU_H

#include "cyclone.h"

typedef struct Cyclone TCycloneContext;

typedef u8 (*TRead8Func)(u32 a);
typedef u16 (*TRead16Func)(u32 a);
typedef u32 (*TRead32Func)(u32 a);

typedef void (*TWrite8Func)(u32 a, u8 d);
typedef void (*TWrite16Func)(u32 a, u16 d);
typedef void (*TWrite32Func)(u32 a, u32 d);

typedef u32 (*TCheckPcFunc)(u32 pc);

typedef struct _TNeoMemoryTableEntry {
	TRead8Func read8;
	TRead16Func read16;
	TRead32Func read32;
	TWrite8Func write8;
	TWrite16Func write16;
	TWrite32Func write32;
	TCheckPcFunc checkPc;
	u32 pad1;
} TNeoMemoryTableEntry;

void cpuInit();
void cpuReset();
void cpuInterrupt(u32 irq);
u32 cpuExecute(s32 cycles);
void cpuDisassemble(char* szText);
void cpuDrawInfo();
u32 cpuGetPC();

void cpuNullWrite8(u32 a, u8 d);
void cpuNullWrite16(u32 a, u16 d);
void cpuNullWrite32(u32 a, u32 d);
u8 cpuUnmapped8();
u16 cpuUnmapped16();
u32 cpuUnmapped32();

u8 neoDefaultRead8(u32 a);
u32 neoDefaultRead32(u32 a);
void neoDefaultWrite32(u32 a, u32 d);

#endif
