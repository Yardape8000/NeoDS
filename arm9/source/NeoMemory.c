#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoCpu.h"
#include "NeoIPC.h"
#include "NeoAudioStream.h"
#include "guiConsole.h"
#include "NeoMemory.h"

#define BANK_COUNT_MIN (256*KB / BANK_CACHE_ENTRY_SIZE)

//#define NEO_AUDIOPROGRAM_SIZE (512*KB)

//STATIC_ASSERT(((BANK_MAX * BANK_CACHE_COUNT * 2) & 3) == 0);

typedef struct _TCacheOwnerEntry {
	//u32 bank; //which bank owns this cache entry?
	u32 index; //which index within that bank owns this entry?
} TCacheOwnerEntry;

static const u8* g_bankTable[BANK_TABLE_SIZE];
static TCacheOwnerEntry* g_cacheOwner;
static u32 g_bankEntry = 0;
static u32 g_bankCacheCount = 0;

//this includes main program and ram
static u8 g_mainProgram[1*MB + 64*KB] ALIGN(32);
static u8 g_bios[128*KB] ALIGN(32);
static u16 g_vram[0x8000] ALIGN(32);
static u16 g_spriteRam[0x800] ALIGN(32);
static u16 g_palette[8*KB] ALIGN(32);
static u8 g_sram[64*KB] ALIGN(32);
static u8 g_card[2*KB] ALIGN(32);
static u8* g_audioProgram;

static u8 g_programVector[128] ALIGN(32);

//neoSystemLoadRegionEx(TNeoRomRegion region, void* pDst, u32 offset, u32 maxSize);

void neoSetRomBankAddr(u32 bankAddr)
{
	//whenever we use this value, we need 0x200000 subtracted off
	//so just do it now to save instructions later
	ASSERTMSG(bankAddr < g_neo->romBankCount * MB, "(%d / %d)", bankAddr, g_neo->romBankCount);
	g_neo->romBankAddress = bankAddr - 0x200000;
}


void neoWriteRomBank(u32 a, u16 d)
{
	if(g_neo->romBankCount == 0) {
		systemWriteLine("Rom has no banks");
		return;
	}

	if((a & 0x00ffffff) >= 0x2ffff0) {
		u32 bank = (d & (BANK_MAX - 1));
		if(bank >= g_neo->romBankCount) {
			systemWriteLine("Invalid bank: %d", bank);
			bank = 0;
		}
		neoSetRomBankAddr(bank * 1*MB);
	}
}

u16 neoReadCard16(u32 a)
{
	//systemWriteLine("neoReadCard16(%06X)", a);
	if((a & 0x00ffffff) <= 0x801000) {
		const u32 d = g_neo->pCard[(a & 0xfff) >> 1] | 0xff00;
		//systemWriteLine(" -> %04X", d);
		return d;
	}
	return 0xffff;
}

void neoWriteCard8(u32 a, u8 d)
{
	//systemWriteLine("neoWriteCard8");
	if((a & 0x00ffffff) <= 0x801000 && !(a & 0x01)) {
		g_neo->pCard[((a & 0xfff) >> 1)] = d;
	}
}

void neoWriteCard16(u32 a, u16 d)
{
	//systemWriteLine("neoWriteCard16(%06X, %02X)", a, d);
	if((a & 0x00ffffff) <= 0x801000) {
		g_neo->pCard[(a & 0xfff) >> 1] = (u8)d;
	}
}

//cacheEntryIndex: which rom page to load
//cacheIndex: where in cache to load it
const u8* neoLoadBank(u32 cacheEntryIndex, u32 cacheIndex)
{
	const u32 loadOffset = 1*MB + cacheEntryIndex * BANK_CACHE_ENTRY_SIZE;
	ASSERT(cacheIndex < g_bankCacheCount);
	ASSERT(cacheEntryIndex < BANK_TABLE_SIZE);
	u8* pEntry = &g_neo->pRom1[cacheIndex << BANK_CACHE_SHIFT];
	TCacheOwnerEntry* pCacheEntry = &g_cacheOwner[cacheIndex];

	//unhook old owner
	ASSERT(pCacheEntry->index < BANK_TABLE_SIZE);
	g_neo->bankTable[pCacheEntry->index] = BANK_CACHE_INVALID;
	//hook new owner
	ASSERT(cacheEntryIndex < BANK_TABLE_SIZE);
	ASSERT(g_neo->bankTable[cacheEntryIndex] == BANK_CACHE_INVALID);
	g_neo->bankTable[cacheEntryIndex] = pEntry;
	//record new owner of this cache block
	pCacheEntry->index = cacheEntryIndex;

	systemWriteLine("neoLoadBank: %d -> %d", cacheEntryIndex, cacheIndex);
	
	neoAudioStreamProcess();

	//load data into cache from fat
	neoSystemReadRegion(NEOROM_MAINPROGRAM, pEntry,
		loadOffset, BANK_CACHE_ENTRY_SIZE);

	return pEntry;
}

//cacheEntryIndex0 where we are loading from ROM
//cacheIndex0 where we are loading to in cache
const u8* neoLoadBank2(u32 cacheEntryIndex0, u32 cacheIndex0)
{
	const u32 cacheIndex1 = cacheIndex0 + 1;
	const u32 cacheEntryIndex1 = cacheEntryIndex0 + 1;
	const u32 loadOffset = 1*MB + cacheEntryIndex0 * BANK_CACHE_ENTRY_SIZE;
	ASSERT(cacheIndex0 < g_bankCacheCount);
	ASSERT(cacheIndex1 < g_bankCacheCount);
	u8* pEntry0 = &g_neo->pRom1[cacheIndex0 << BANK_CACHE_SHIFT];
	u8* pEntry1 = &g_neo->pRom1[cacheIndex1 << BANK_CACHE_SHIFT];
	TCacheOwnerEntry* pCacheEntry0 = &g_cacheOwner[cacheIndex0];
	TCacheOwnerEntry* pCacheEntry1 = &g_cacheOwner[cacheIndex1];

	if(pCacheEntry1->index == cacheEntryIndex1) {
		//shortcut if second bank is already in place, or if we are loading memory
		//in from end of rom, and we don't need second bank
		systemWriteLine(" -> switch to single bank");
		return neoLoadBank(cacheEntryIndex0, cacheIndex0);
	}

	//unhook old owner
	ASSERT(pCacheEntry0->index < BANK_TABLE_SIZE);
	ASSERT(pCacheEntry0->index < BANK_TABLE_SIZE);
	g_neo->bankTable[pCacheEntry0->index] = BANK_CACHE_INVALID;
	g_neo->bankTable[pCacheEntry1->index] = BANK_CACHE_INVALID;
	//hook new owner
	ASSERT(cacheEntryIndex0 < BANK_TABLE_SIZE);
	ASSERT(cacheEntryIndex1 < BANK_TABLE_SIZE);
	ASSERT(g_neo->bankTable[cacheEntryIndex0] == BANK_CACHE_INVALID);
	ASSERT(g_neo->bankTable[cacheEntryIndex1] == BANK_CACHE_INVALID);
	g_neo->bankTable[cacheEntryIndex0] = pEntry0;
	g_neo->bankTable[cacheEntryIndex1] = pEntry1;
	//record new owner of this cache block
	pCacheEntry0->index = cacheEntryIndex0;
	pCacheEntry1->index = cacheEntryIndex1;

	systemWriteLine("neoLoadBank2: %d -> %d", cacheEntryIndex0, cacheIndex0);

	neoAudioStreamProcess();

	//load data into cache from fat
	neoSystemReadRegion(NEOROM_MAINPROGRAM, pEntry0,
		loadOffset, BANK_CACHE_ENTRY_SIZE * 2);

	return pEntry0;
}

u32 neoEvictBank()
{
	g_bankEntry++;
	if(g_bankEntry >= g_bankCacheCount) g_bankEntry = 0;
	//systemWriteLine("neoEvictBank: %d", g_bankEntry);
	return g_bankEntry;
}

u32 neoEvictBank2()
{
	g_bankEntry++;
	if(g_bankEntry >= g_bankCacheCount - 1) g_bankEntry = 0;
	//systemWriteLine("neoEvictBank2: %d", g_bankEntry);
	return g_bankEntry;
}

u32 neoBankPC(u32 a)
{
	const u32 address = (a & 0x00ffffff) + g_neo->romBankAddress;
	const u32 cacheEntryIndex0 =
		(address >> BANK_CACHE_SHIFT) & (BANK_TABLE_SIZE - 1);
	const u8* pEntry0 = g_neo->bankTable[cacheEntryIndex0];
	u32 cacheIndex0 = (u32)(pEntry0 - g_neo->pRom1) >> BANK_CACHE_SHIFT;

	ASSERT(cacheIndex0 != g_bankCacheCount - 1);

	if(pEntry0 == BANK_CACHE_INVALID || cacheIndex0 == g_bankCacheCount - 1) {
		//if the first bank doesn't match, or if it occurs at the end of the cache,
		//we need to load 2 consecutive banks
		cacheIndex0 = neoEvictBank2();
		systemWriteLine("Bank pc: %06X", a);
		systemWriteLine(" -> %d to %d", cacheEntryIndex0, cacheIndex0);
		pEntry0 = neoLoadBank2(cacheEntryIndex0, cacheIndex0);
	} else {
		const u32 cacheIndex1 = cacheIndex0 + 1;
		const u32 cacheEntryIndex1 = cacheEntryIndex0 + 1;
		//if the first bank matches, make sure the second bank matches
		if(g_cacheOwner[cacheIndex1].index != cacheEntryIndex1) {
			//load it if it doesn't
			systemWriteLine("Half Bank pc: %06X", a);
			systemWriteLine(" -> %d to %d", cacheEntryIndex1, cacheIndex1);
			neoLoadBank(cacheEntryIndex1, cacheIndex1);
		}
	}
	return (u32)pEntry0 - (a & ~(BANK_CACHE_ENTRY_SIZE - 1));
}

u32 neoBankedRomPc(u32 pc)
{
	g_neo->cpu.membase = neoBankPC(pc);
	return (u32)g_neo->cpu.membase + pc;
}

void neoMemoryLoadBiosVector()
{
	systemWriteLine("Load BIOS Vector");
	memcpy(g_neo->pRom0, g_neo->pBios, 128);
}

void neoMemoryLoadProgramVector()
{
	systemWriteLine("Load PROGRAM Vector");
	memcpy(g_neo->pRom0, g_programVector, 128);
	//HACK
	g_neo->sramProtectCount++;
}

void neoMemoryInit()
{
	u32 i;

	systemWriteLine("Memory initialized");
	//systemWriteLine(" -> BANK_CACHE_COUNT: %d", BANK_CACHE_COUNT);
	//systemWriteLine(" -> BANK_CACHE_ENTRY_SIZE: %d", BANK_CACHE_ENTRY_SIZE);

	g_neo->pRom0 = g_mainProgram;
	g_neo->pBios = g_bios;
	g_neo->pRam = &g_mainProgram[1*MB];
	g_neo->pSram = g_sram;
	g_neo->pCard = g_card;
	g_neo->pVram = g_vram;
	g_neo->pSpriteRam = g_spriteRam;
	g_neo->pPalette = g_palette;
	//g_neo->pNitroPalette = g_nitroPalette;
	g_neo->bankTable = g_bankTable;

	for(i = 0; i < BANK_TABLE_SIZE; i++) {
		g_neo->bankTable[i] = BANK_CACHE_INVALID;
	}

	for(i = 0; i < 64*KB; i++) {
		g_neo->pRam[i] = 0;
	}

	//load audio rom
	const u32 audioProgramSize = neoSystemRegionSize(NEOROM_AUDIOPROGRAM);
	g_audioProgram = systemRamAlloc(audioProgramSize);
	neoSystemLoadRegion(NEOROM_AUDIOPROGRAM, UNCACHED(g_audioProgram), audioProgramSize);
	NEOIPC->audioProgramSize = audioProgramSize;
	NEOIPC->pAudioProgram0 = UNCACHED(g_audioProgram);
	NEOIPC->audioRomSize = neoSystemRegionSize(NEOROM_AUDODATA);
	
	if(g_neo->romBankCount == 0) {
		guiConsoleLog("No rom banks...");
		for(i = 0x20; i < 0x30; i++) {
			g_neo->cpuRead8Table[i] = neoReadRom08;
			g_neo->cpuRead16Table[i] = neoReadRom016;
			g_neo->cpuRead32Table[i] = neoReadRom032;
			g_neo->cpuCheckPcTable[i] = neoRom0Pc;
		}
		g_bankCacheCount = 0;
		g_neo->pRom1 = 0;
		g_cacheOwner = 0;
	} else {
		u32 bankCount = 1*MB * g_neo->romBankCount / BANK_CACHE_ENTRY_SIZE;
		while(bankCount >= BANK_COUNT_MIN) {
			if(systemGetRamFree() >= bankCount * (BANK_CACHE_ENTRY_SIZE + sizeof(TCacheOwnerEntry))) {
				break;
			}
			bankCount >>= 1;
		}
		guiConsoleLogf("Allocate %d bank cache entries", bankCount);
		g_bankCacheCount = bankCount;
		g_neo->pRom1 = systemRamAlloc(bankCount * BANK_CACHE_ENTRY_SIZE);
		g_cacheOwner = (TCacheOwnerEntry*)systemRamAlloc(bankCount * sizeof(TCacheOwnerEntry));

		for(i = 0; i < g_bankCacheCount; i++) {
			g_bankTable[i] = &g_neo->pRom1[i << BANK_CACHE_SHIFT];
			g_cacheOwner[i].index = i;
		}

		guiConsoleLogf("Loading banks into main ram...");
		neoSystemLoadRegionEx(NEOROM_MAINPROGRAM, g_neo->pRom1, 1*MB, g_bankCacheCount * BANK_CACHE_ENTRY_SIZE);

		if(bankCount * BANK_CACHE_ENTRY_SIZE == 1*MB * g_neo->romBankCount) {
			guiConsoleLogf(" -> no banking needed");
			for(i = 0x20; i < 0x30; i++) {
				g_neo->cpuRead8Table[i] = neoReadRom18;
				g_neo->cpuRead16Table[i] = neoReadRom116;
				g_neo->cpuRead32Table[i] = neoReadRom132;
				g_neo->cpuCheckPcTable[i] = neoRom1Pc;
			}
		} else {
			for(i = 0x20; i < 0x30; i++) {
				g_neo->cpuRead8Table[i] = neoReadBankedRom8;
				g_neo->cpuRead16Table[i] = neoReadBankedRom16;
				g_neo->cpuRead32Table[i] = neoReadBankedRom32;
				g_neo->cpuCheckPcTable[i] = neoBankedRomPc;
			}
		}
	}

	neoSystemLoadRegion(NEOROM_MAINPROGRAM, g_neo->pRom0, 1*MB);
	neoSystemLoadRegion(NEOROM_BIOS, g_neo->pBios, 128*KB);
	memcpy(g_programVector, g_neo->pRom0, 128);
	neoMemoryLoadBiosVector();

	//$10FD83     Nationality of the machine (0 = Japanese / 1 & 2 = English)
	//$10FDAE     Set to zero before booting to force complete initialization.
	//$10FE80     Set to FF to activate debug mode.
	//g_neo->pRam[0xFD83] = 0; //Japanese
	//g_neo->pRam[0xFDAE] = 1; //skip long boot

	if(g_neo->romBankCount > 0) {
		neoSetRomBankAddr(0);
	}

	systemWriteLine(" -> AudioProgram size: %d", NEOIPC->audioProgramSize);
	systemWriteLine(" -> AudioRom size: %d", NEOIPC->audioRomSize);

	guiConsoleDump();
}
