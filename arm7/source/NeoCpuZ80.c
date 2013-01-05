#include <string.h> //for memset
#include "nds.h"
#include "NeoIPC.h"
#include "NeoSystem7.h"
#include "NeoYM2610.h"
#include "NeoCpuZ80.h"

static u32 g_audioProgramSize;
static u8* g_pAudioProgram;
static u8 g_z80Rom[32*KB];
static u8 g_audioCommand = 0;

u8 neoZ80Read8(u16 a);
u16 neoZ80Read16(u16 a);
void neoZ80Write8(u8 d, u16 a);
void neoZ80Write16(u16 d, u16 a);

unsigned int neoZ80RebasePC(unsigned short a)
{
	const u32 offset = (a & ~(MEMBLOCK_SIZE - 1));
	g_neo7->z80.Z80PC_BASE = (u32)g_neo7->z80MemTable[a >> MEMBLOCK_SHIFT] - offset;
	return g_neo7->z80.Z80PC_BASE + a;
}

unsigned int neoZ80RebaseSP(unsigned short a)
{
	const u32 offset = (a & ~(MEMBLOCK_SIZE - 1));
	g_neo7->z80.Z80SP_BASE = (u32)g_neo7->z80MemTable[a >> MEMBLOCK_SHIFT] - offset;
	return g_neo7->z80.Z80SP_BASE + a;
}

void neoZ80Bank(u8 bank, u16 a)
{
	const u32 mask = (g_audioProgramSize - 1) & 0x3ffff;
	const u32 addr = (((a & 0xff00) << (3 + bank)) & mask);
	u8* pSrc = g_pAudioProgram + addr;
	u32 i;

    switch(bank) {
    case 3:
		for(i = 0; i < 8; i++) {
			g_neo7->z80MemTable[16 + i] = pSrc;
			pSrc += MEMBLOCK_SIZE;
		}
		break;
    case 2:
		for(i = 0; i < 4; i++) {
			g_neo7->z80MemTable[24 + i] = pSrc;
			pSrc += MEMBLOCK_SIZE;
		}
		break;
    case 1:
		g_neo7->z80MemTable[28] = pSrc;
		g_neo7->z80MemTable[29] = pSrc + MEMBLOCK_SIZE;
		break;
    case 0:
		g_neo7->z80MemTable[30] = pSrc;
		break;
    }
}

u8 neoZ80In(u16 a)
{
	switch(a & 0xff) {
    case 0x0:
		//return sound_code;
		neoZ80ClearNmi();
		NEOIPC->audioCommandPending = 0;
		return g_audioCommand;
    case 0x4:
		return neoYM2610Read(0);
    case 0x5:
		return neoYM2610Read(1);
    case 0x6:
		return neoYM2610Read(2);
    case 0x08:
		neoZ80Bank(0, a);
		return 0;
    case 0x09:
		neoZ80Bank(1, a);
		return 0;
    case 0x0a:
		neoZ80Bank(2, a);
		return 0;
    case 0x0b:
		neoZ80Bank(3, a);
		return 0;
    }
    return 0;
}

void neoZ80Out(u16 a, u8 d)
{
	switch(a & 0xff) {
    case 0x4:
		neoYM2610Write(0, d);
		break;
    case 0x5:
		neoYM2610Write(1, d);
		break;
    case 0x6:
		neoYM2610Write(2, d);
		break;
    case 0x7:
		neoYM2610Write(3, d);
		break;
    case 0xC:
		NEOIPC->audioResult = d;
		break;
	}
}

void neoZ80Init()
{
	memset(&g_neo7->z80, 0, sizeof(g_neo7->z80));
	g_neo7->z80.z80_rebasePC = neoZ80RebasePC;
	g_neo7->z80.z80_rebaseSP = neoZ80RebaseSP;
	g_neo7->z80.z80_read8 = neoZ80Read8;
	g_neo7->z80.z80_read16 = neoZ80Read16;
	g_neo7->z80.z80_write8 = neoZ80Write8;
	g_neo7->z80.z80_write16 = neoZ80Write16;
	g_neo7->z80.z80_in = neoZ80In;
	g_neo7->z80.z80_out = neoZ80Out;
	g_neo7->z80.z80irqvector = 0x38;

	g_neo7->z80.Z80A = 0x00 <<24;
	g_neo7->z80.Z80F = (1<<2); /* set ZFlag */
	g_neo7->z80.Z80BC = 0x0000 <<16;
	g_neo7->z80.Z80DE = 0x0000 <<16;
	g_neo7->z80.Z80HL = 0x0000 <<16;
	g_neo7->z80.Z80A2 = 0x00 <<24;
	g_neo7->z80.Z80F2 = 1<<2;  /* set ZFlag */
	g_neo7->z80.Z80BC2 = 0x0000 <<16;
	g_neo7->z80.Z80DE2 = 0x0000 <<16;
	g_neo7->z80.Z80HL2 = 0x0000 <<16;
	g_neo7->z80.Z80IX = 0xFFFF;// <<16;
	g_neo7->z80.Z80IY = 0xFFFF;// <<16;
	g_neo7->z80.Z80I = 0x00;
	g_neo7->z80.Z80IM = 0x01;
	g_neo7->z80.Z80_IRQ = 0x00;
	g_neo7->z80.Z80IF = 0x00;
	//g_neo7->z80.Z80PC = g_neo7->z80.z80_rebasePC(0);
	//g_neo7->z80.Z80SP = g_neo7->z80.z80_rebaseSP(0xffff);
	neoZ80Reset();
}

void neoZ80Reset()
{
	u8* pSrc = g_z80Rom;
	u32 i;

	g_pAudioProgram = NEOIPC->pAudioProgram0;
	g_audioProgramSize = NEOIPC->audioProgramSize;

	//copy the first 32KB of the z80 rom into arm7 memory
	memcpy(pSrc, g_pAudioProgram, 32*KB);
	for(i = 0; i < 16; i++) {
		g_neo7->z80MemTable[i] = pSrc;
		pSrc += MEMBLOCK_SIZE;
	}

	pSrc = g_pAudioProgram + 32*KB;
	for(; i < 32; i++) {
		g_neo7->z80MemTable[i] = pSrc;
		pSrc += MEMBLOCK_SIZE;
	}

	//final 2KB entry points at ram
	g_neo7->z80MemTable[31] = g_neo7->z80Ram;

	g_neo7->z80.Z80PC = g_neo7->z80.z80_rebasePC(0);
	g_neo7->z80.Z80SP = g_neo7->z80.z80_rebaseSP(0xffff);
}

void neoZ80ClearNmi()
{
	//g_neo7->z80.Z80_NMI = 0;
	g_neo7->z80.Z80_IRQ &= ~2;
}

void neoZ80Nmi()
{
	g_audioCommand = NEOIPC->audioCommand;
	//g_neo7->z80.Z80_NMI = 1;
	g_neo7->z80.Z80_IRQ |= 2;
}

void neoZ80ClearIrq()
{
	//g_neo7->z80.Z80_IRQ = 0;
	g_neo7->z80.Z80_IRQ &= ~1;
}

void neoZ80Irq()
{
	//g_neo7->z80.Z80_IRQ = 1;
	g_neo7->z80.Z80_IRQ |= 1;
}

s32 neoZ80Execute(s32 cycles)
{
	DrZ80Run(&g_neo7->z80, cycles);
	return cycles;
}
