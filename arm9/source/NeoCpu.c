#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoCpu.h"
#include "NeoVideo.h"
#include "NeoIO.h"
#include "NeoMemory.h"
#include "Disa.h"

void cpuNullWrite8(u32 a, u8 d) {}
void cpuNullWrite16(u32 a, u16 d) {}
void cpuNullWrite32(u32 a, u32 d) {}

u8 cpuUnmapped8() { return 0xf3; }
u16 cpuUnmapped16() { return 0xff3f; }
u32 cpuUnmapped32() { return 0xffffff3f; }

extern unsigned int neoCpuRead8(unsigned int a);
extern unsigned int neoCpuRead16(unsigned int a);
extern unsigned int neoCpuRead32(unsigned int a);
	
extern void neoCpuWrite8(unsigned int a, unsigned char d);
extern void neoCpuWrite16(unsigned int a, unsigned short d);
extern void neoCpuWrite32(unsigned int a, unsigned int d);

extern unsigned int neoCpuCheckPc(unsigned int pc);

/*u32 neoCpuRead8(u32 a)
{
	return g_neo->cpuMemTable[a >> 16].read8(a);
}

u32 neoCpuRead16(u32 a)
{
	return g_neo->cpuMemTable[a >> 16].read16(a);
}

u32 neoCpuRead32(u32 a)
{
	return g_neo->cpuMemTable[a >> 16].read32(a);
}
	
void neoCpuWrite8(u32 a, u8 d)
{
	g_neo->cpuMemTable[a >> 16].write8(a, d);
}

void neoCpuWrite16(u32 a, u16 d)
{
	g_neo->cpuMemTable[a >> 16].write16(a, d);
}

void neoCpuWrite32(u32 a, u32 d)
{
	g_neo->cpuMemTable[a >> 16].write32(a, d);
}*/

/*static u8 neoReadSram8(u32 a)
{
	return CPU_READ8(g_neo->pSram, a & 0xffff);
}

static u16 neoReadSram16(u32 a)
{
	return CPU_READ16(g_neo->pSram, a & 0xffff);
}

static u32 neoReadSram32(u32 a)
{
	return CPU_READ32(g_neo->pSram, a & 0xffff);
}*/

static u32 neoInvalidPc(u32 pc)
{
	systemPanic("Invalid PC: %08X", pc);
	return 0;
}

void neoCpuInitMemoryTable()
{
	u32 i;

	for(i = 0; i < 256; i++) {
		g_neo->cpuRead8Table[i] = neoDefaultRead8;
		g_neo->cpuRead16Table[i] = cpuUnmapped16;
		g_neo->cpuRead32Table[i] = neoDefaultRead32;
		g_neo->cpuWrite8Table[i] = cpuNullWrite8;
		g_neo->cpuWrite16Table[i] = cpuNullWrite16;
		g_neo->cpuWrite32Table[i] = neoDefaultWrite32;
		g_neo->cpuCheckPcTable[i] = neoInvalidPc;
	}
	
	for(i = 0; i < 0x10; i++) {
		g_neo->cpuRead8Table[i] = neoReadRom8;
		g_neo->cpuRead16Table[i] = neoReadRom16;
		g_neo->cpuRead32Table[i] = neoReadRom32;
		g_neo->cpuCheckPcTable[i] = neoRomPc;
	}
	
	for(i = 0x10; i < 0x20; i++) {
		g_neo->cpuRead8Table[i] = neoReadRam8;
		g_neo->cpuRead16Table[i] = neoReadRam16;
		g_neo->cpuRead32Table[i] = neoReadRam32;
		g_neo->cpuWrite8Table[i] = neoWriteRam8;
		g_neo->cpuWrite16Table[i] = neoWriteRam16;
		g_neo->cpuWrite32Table[i] = neoWriteRam32;
		g_neo->cpuCheckPcTable[i] = neoRamPc;
	}
	
	g_neo->cpuWrite8Table[0x2f] = (TWrite8Func)neoWriteRomBank;
	g_neo->cpuWrite16Table[0x2f] = neoWriteRomBank;

	//io read
	g_neo->cpuRead16Table[0x30] = neoReadCtrl116;
	g_neo->cpuRead16Table[0x34] = neoReadCtrl216;
	g_neo->cpuRead16Table[0x32] = neoReadCoin16;
	g_neo->cpuRead16Table[0x38] = neoReadCtrl316;

	for(i = 0x3c; i <= 0x3d; i++) {
		g_neo->cpuRead8Table[i] = neoReadVideo8;
		g_neo->cpuRead16Table[i] = neoReadVideo16;
		g_neo->cpuRead32Table[i] = neoReadVideo32;
	}

	//io write
	for(i = 0x30; i <= 0x31; i++) {
		g_neo->cpuWrite8Table[i] = neoWriteWatchdog8;
		g_neo->cpuWrite16Table[i] = neoWriteWatchdog16;
	}

	for(i = 0x32; i <= 0x33; i++) {
		g_neo->cpuWrite8Table[i] = neoWriteAudioCommand8;
		g_neo->cpuWrite16Table[i] = neoWriteAudioCommand16;
	}

	g_neo->cpuWrite8Table[0x38] = (TWrite8Func)neoWrite4990a16;
	g_neo->cpuWrite16Table[0x38] = neoWrite4990a16;

	for(i = 0x3a; i <= 0x3b; i++) {
		g_neo->cpuWrite8Table[i] = (TWrite8Func)neoWriteSystemLatch16;
		g_neo->cpuWrite16Table[i] = neoWriteSystemLatch16;
	}

	for(i = 0x3c; i <= 0x3d; i++) {
		g_neo->cpuWrite8Table[i] = neoWriteVideo8;
		g_neo->cpuWrite16Table[i] = neoWriteVideo16;
		g_neo->cpuWrite32Table[i] = neoWriteVideo32;
	}
	
	//palette
	for(i = 0x40; i < 0x70; i++) {
		g_neo->cpuRead16Table[i] = neoVideoReadPal16;
		g_neo->cpuWrite8Table[i] = neoVideoWritePal8;
		g_neo->cpuWrite16Table[i] = neoVideoWritePal16;
	}

	//memory card
	g_neo->cpuRead16Table[0x80] = neoReadCard16;
	g_neo->cpuWrite8Table[0x80] = neoWriteCard8;
	g_neo->cpuWrite16Table[0x80] = neoWriteCard16;
	
	//bios
	for(i = 0xC0; i < 0xD0; i++) {
		g_neo->cpuRead8Table[i] = neoReadBios8;
		g_neo->cpuRead16Table[i] = neoReadBios16;
		g_neo->cpuRead32Table[i] = neoReadBios32;
		g_neo->cpuCheckPcTable[i] = neoBiosPc;
	}
	
	//sram
	for(i = 0xD0; i < 0xE0; i++) {
		g_neo->cpuRead8Table[i] = neoReadSram8;
		g_neo->cpuRead16Table[i] = neoReadSram16;
		g_neo->cpuRead32Table[i] = neoReadSram32;
		g_neo->cpuWrite8Table[i] = neoWriteSram8;
		g_neo->cpuWrite16Table[i] = neoWriteSram16;
		//g_neo->cpuWrite32Table[i] = neoWriteSram32;
	}
}

/*static const char* cpuGetMemBase()
{
	if(g_neo->cpu.membase == (u32)g_neo->pRom0) {
		return "MainProgram";
	} else if(g_neo->cpu.membase == (u32)g_neo->pRam - 0x100000) {
		return "Ram";
	} else if(g_neo->cpu.membase == (u32)g_neo->pBios - 0xC00000) {
		return "Bios";
	} else if(g_neo->cpu.membase >= (u32)g_neo->pRom1 - 0x200000 &&
		g_neo->cpu.membase < (u32)g_neo->pRom1 - 0x100000) {
			return "BankedProgram";
	} else {
		return "Unknown";
	}
}*/

u32 neoDebugCpuCheckPc(u32 pc)
{
	/*
	ldr r2, [r7, #NEO_CPU_MEMBASE] //r2 = membase
	sub r0, r0, r2 //r0 = pc - membase (r0 = actual pc)
	and r2, r0, #0x00ff0000
	add r3, r7, #NEO_CHECKPCTABLE
	ldr pc, [r3, r2, lsr #14]
	*/
	const u32 actualPc = pc - g_neo->cpu.membase;
	const u32 index = (actualPc & 0x00ff0000) >> 16;
	systemWriteLine("neoDebugCpuCheckPc: %06X", actualPc);
	systemWriteLine(" -> index: %d", index);
	if(g_neo->cpuCheckPcTable[index] == neoInvalidPc) {
		systemWriteLine(" -> neoInvalidPc");
	} else if(g_neo->cpuCheckPcTable[index] == neoBiosPc) {
		systemWriteLine(" -> neoBiosPc");
	} else if(g_neo->cpuCheckPcTable[index] == neoRom0Pc) {
		systemWriteLine(" -> neoRom0Pc");
	} else if(g_neo->cpuCheckPcTable[index] == 0) {
		systemWriteLine(" -> NULL");
	} else {
		systemWriteLine(" -> %08X???", (u32)g_neo->cpuCheckPcTable[index]);
	}
	const u32 ret = g_neo->cpuCheckPcTable[index](actualPc);
	systemWriteLine(" -> %08X", ret);
	return ret;
}

void cpuInit()
{
	neoCpuInitMemoryTable();

	memset(&g_neo->cpu, 0, sizeof(TCycloneContext));

	g_neo->cpu.checkpc = neoCpuCheckPc; 
	/*g_neo->cpu.read8 = cpuRead8;
	g_neo->cpu.read16 = cpuRead16;
	g_neo->cpu.read32 = cpuRead32;
	g_neo->cpu.write8 = cpuWrite8;
	g_neo->cpu.write16 = cpuWrite16;
	g_neo->cpu.write32 = cpuWrite32;
	g_neo->cpu.fetch8 = cpuRead8;
	g_neo->cpu.fetch16 = cpuRead16;
	g_neo->cpu.fetch32 = cpuRead32;*/
	g_neo->cpu.read8 = neoCpuRead8;
	g_neo->cpu.read16 = neoCpuRead16;
	g_neo->cpu.read32 = neoCpuRead32;
	g_neo->cpu.write8 = neoCpuWrite8;
	g_neo->cpu.write16 = neoCpuWrite16;
	g_neo->cpu.write32 = neoCpuWrite32;
	g_neo->cpu.fetch8 = neoCpuRead8;
	g_neo->cpu.fetch16 = neoCpuRead16;
	g_neo->cpu.fetch32 = neoCpuRead32;
#ifndef NEO_SHIPPING
	DisaWord = neoCpuRead16;
#endif

	systemWriteLine("CPU initialized");
	systemWriteLine(" -> sizeof(TCycloneContext): %04X", sizeof(TCycloneContext));
}

void cpuReset()
{
	g_neo->cpu.srh = 0x27; // Set supervisor mode
	g_neo->cpu.a[7] = g_neo->cpu.read32(0); // Get Stack Pointer
	g_neo->cpu.membase = 0;
	g_neo->cpu.pc = g_neo->cpu.checkpc(g_neo->cpu.read32(4)); // Get Program Counter

	systemWriteLine("CPU reset");
	systemWriteLine(" -> PC: %06X (%s)",
		g_neo->cpu.pc - g_neo->cpu.membase, cpuGetMemBase());
	systemWriteLine(" -> SP: %06X", g_neo->cpu.a[7]);
}

void cpuDisassemble(char* szText)
{
#ifndef NEO_SHIPPING
	DisaText = szText;
	DisaPc = g_neo->cpu.pc - g_neo->cpu.membase;
	DisaGet();
#endif
}

void cpuInterrupt(u32 irq)
{
	g_neo->cpu.irq = irq;
}

//#define BREAKPOINT 0xc00402

u32 cpuGetPC()
{
	return g_neo->cpu.pc - g_neo->cpu.membase;
}

u32 cpuExecute(s32 cycles)
{
#ifdef BREAKPOINT
	g_neo->debug = true;
	if(g_neo->debug) {
		systemWriteLine("Wait for key...");
		u32 keys = 0;
		while(keys == 0) {
			swiWaitForVBlank();
			scanKeys();
			keys = keysDown();
			if(keys & KEY_START) {
				g_neo->debug = false;
			}
		}
		systemWriteLine(" -> Pressed");
	}
	
	const s32 initCycles = cycles;
	while(cycles > 0) {
		if(g_neo->debug) {
			cpuDrawInfo();
			u32 keys = 0;
			systemWriteLine("Wait for key...");
			while(keys == 0) {
				swiWaitForVBlank();
				scanKeys();
				keys = keysDown();
				if(keys & KEY_START) {
					g_neo->debug = false;
				}
			}
			systemWriteLine(" -> Pressed");
		}
		g_neo->cpu.cycles = 0;
		CycloneRun(&g_neo->cpu);
		cycles += g_neo->cpu.cycles;
		if(g_neo->cpu.pc - g_neo->cpu.membase == BREAKPOINT) {
			g_neo->debug = true;
		}
	}
	return initCycles - cycles;
#else
	g_neo->cpu.cycles = cycles;
	CycloneRun(&g_neo->cpu);
	const u32 ranCycles = cycles - g_neo->cpu.cycles;
	return ranCycles;
#endif

}
