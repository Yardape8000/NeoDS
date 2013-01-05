#ifndef _NEO_CPUZ80_H
#define _NEO_CPUZ80_H

#define MEMBLOCK_SHIFT 11
#define MEMBLOCK_SIZE (1 << MEMBLOCK_SHIFT)
#define MEMBLOCK_COUNT 32

#ifndef NEO_IN_ASM

#include "DrZ80.h"

typedef struct DrZ80 TDrZ80Context;

void neoZ80Init();
void neoZ80Nmi();
void neoZ80ClearNmi();
void neoZ80Irq();
void neoZ80ClearIrq();
s32 neoZ80Execute(s32 cycles);
void neoZ80Reset();

#endif //NEO_IN_ASM

#endif
