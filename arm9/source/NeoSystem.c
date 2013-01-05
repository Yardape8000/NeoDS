#include "Default.h"
#include <fat.h>
#include <sys/dir.h>
#include <stdlib.h> //for qsort

#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoConfig.h"
#include "NeoSystemAsm.h"
#include "pd4990a.h"
#include "NeoCpu.h"
#include "NeoVideo.h"
#include "NeoMemory.h"
#include "NeoIPC.h"
#include "NeoAudioStream.h"
#include "NeoProfiler.h"
#include "LinearHeap.h"
#include "guiConsole.h"
#include "guiBase.h"

#define NEO_ROM_MAX 64

#define NEO_HEADER_SIZE 512

STATIC_ASSERT(sizeof(TNeoContext) == NEO_CONTEXTEND);
//verifty that the variable section of TNeoContext lines up (without the mem tables)
STATIC_ASSERT(OFFSET(TNeoContext, varEnd) == NEO_VAREND);
//verifty that varEnd is less than 0x400
STATIC_ASSERT(OFFSET(TNeoContext, cpuRead8Table) == 0x400);

TNeoContext g_neoContext NEO_SYSTEM_SECTION;
//TNeoContext* g_neo DTCM_BSS;

TNeoRomHeader g_header;
static int g_rom = -1;
static u32 g_romSize = 0;
static TLinearHeap g_vramHHeap;

static u32 g_romCount = 0;
static char* g_romNames[NEO_ROM_MAX];
static s32 g_pauseCount = 0;
static bool g_paused = false;

#ifndef NEO_SHIPPING
static const char* const g_neoRomRegionNames[] = {
	"MainProgram",
	"Bios",
	"AudioProgram",
	"FixedData",
	"AudioData1",
	"AudioData2",
	"AudioData3",
	"AudioData4",
	"SpriteData",
};
#endif

void neoSystemIPCSync()
{
	/*u32 command = IPC_GetSync();
	switch(command) {
	case NEOARM9_READAUDIO:
		neoIPCRecvCommand();
		NEOIPC->arm9Return = neoAudioStream(NEOIPC->arm9Args[0], &NEOIPC->arm9Args[1]);
		neoIPCAckCommand();
		break;
	}*/
}

void neoSystemReadRom(u8* pDst, u32 offset, u32 size)
{
	profilerPush(NEOPROFILE_ROMREAD);
	systemReadOffset(g_rom, pDst, offset, size);
	profilerPop();
}

void neoSystemLoadSprite(u8* pDst, u32 index)
{
	const u32 romOffset =
		g_header.romEntry[NEOROM_SPRITEDATA].offset +
		index * SPRITE_CACHE_ENTRY_SIZE;
	neoSystemReadRom(pDst, romOffset, SPRITE_CACHE_ENTRY_SIZE);
}

void neoSystemLoadSprite2(u8* pDst, u32 index)
{
	const u32 romOffset =
		g_header.romEntry[NEOROM_SPRITEDATA].offset +
		index * SPRITE_CACHE2_ENTRY_SIZE;
	neoSystemReadRom(pDst, romOffset, SPRITE_CACHE2_ENTRY_SIZE);
}

void neoSystemLoadTile(u8* pDst, u32 index)
{
	const u32 romOffset =
		g_header.romEntry[NEOROM_FIXEDDATA].offset +
		index * TILE_CACHE_ENTRY_SIZE;
	neoSystemReadRom(pDst, romOffset, TILE_CACHE_ENTRY_SIZE);
}

void neoSystemLoadRegionEx(TNeoRomRegion region, void* pDst, u32 offset, u32 maxSize)
{
	const TNeoRomEntry* pRegion = &g_header.romEntry[(u32)region];
	const u32 fileOffset = pRegion->offset + offset;
	u32 size = maxSize;
	if(offset >= pRegion->size) return;
	if(offset + size > pRegion->size) size = pRegion->size - offset;

	if(pRegion->offset == 0xffffffff || pRegion->size == 0xffffffff) {
		systemWriteLine("Undefined region");
		return;
	}
	neoSystemReadRom(pDst, fileOffset, size);
}

void neoSystemReadRegion(TNeoRomRegion region, void* pDst, u32 offset, u32 size)
{
	const TNeoRomEntry* pRegion = &g_header.romEntry[(u32)region];
	const u32 fileOffset = pRegion->offset + offset;
	neoSystemReadRom(pDst, fileOffset, size);
}

u32 neoSystemRegionSize(TNeoRomRegion region)
{
	const TNeoRomEntry* pRegion = &g_header.romEntry[(u32)region];
	return pRegion->size;
}

void neoSystemLoadRegion(TNeoRomRegion region, void* pDst, u32 maxSize)
{
	neoSystemLoadRegionEx(region, pDst, 0, maxSize);
}

void neoInstallProtection();

void* neoSystemVramHAlloc(u32 size)
{
	return linearHeapAlloc(&g_vramHHeap, size);
}

void neoSystemSetClockDivide(u32 clock)
{
	g_neo->cpuClockDivide = clock;
	g_neo->cpuClocksPerScanline = CPU_CLOCKS_PER_SCANLINE;
}

/*typedef void (*TRomFileIterator)(const char* szFileName, void* arg);

static bool romFileIterate(TRomFileIterator iterator, void* arg)
{
	DIR_ITER* dir;
	struct stat st;
	char szFilename[256];

	dir = diropen("/");
	if(!dir) {
		return false;
	}

	while(dirnext(dir, szFilename, &st) == 0) {
		if(st.st_mode & S_IFDIR) {
			continue;
		}
		const char* szExt = strchr(szFilename, '.');
		if(strcmpi(".NEO", szExt)) {
			continue;
		}
		iterator(szFilename, arg);
	}

	dirclose(dir);
	return true;
}

static void romCountIterator(const char* szFileName, void* arg)
{
	s32* pCount = (s32*)arg;
	*pCount = *pCount + 1;
}

static void romAddMenuIterator(const char* szFileName, void* arg)
{
	TGuiMenu* pMenu = (TGuiMenu*)arg;
	guiMenuAddItem(pMenu, szFileName);
}*/

int stringCompare(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}

bool neoSystemInit()
{
	irqSet(IRQ_IPC_SYNC, neoSystemIPCSync);

	linearHeapInit(&g_vramHHeap, (void*)0x6898000, 32*KB);

	systemWriteLine("sizeof(TNeoContext): %d", sizeof(TNeoContext));
	systemWriteLine(" -> varEnd: %d", OFFSET(TNeoContext, varEnd));

	g_romCount = 0;
	memset(g_romNames, 0, sizeof(g_romNames));

	DIR_ITER* dir;
	struct stat st;
	char szFilename[256];

	dir = diropen("/");
	if(!dir) {
		return false;
	}

	while(g_romCount < NEO_ROM_MAX && dirnext(dir, szFilename, &st) == 0) {
		if(st.st_mode & S_IFDIR) {
			continue;
		}
		const char* szExt = strchr(szFilename, '.');
		if(strcmpi(".NEO", szExt)) {
			continue;
		}
		
		g_romCount++;
		g_romNames[g_romCount - 1] = strdup(szFilename);
		ASSERT(g_romNames[g_romCount - 1]);
	}
	qsort(g_romNames, g_romCount, sizeof(const char*), stringCompare);

	dirclose(dir);

	neoResetContext();
	neoSystemSetClockDivide(2);

	return true;
}

u32 neoSystemGetRomCount()
{
	return g_romCount;
}

const char* neoSystemGetRomName(u32 i)
{
	ASSERT(i < g_romCount);
	return g_romNames[i];
}

bool neoSystemOpen(const char* szFileName)
{
	TNeoRomHeader header;
	int rom;
	u32 bit;

	ASSERT(g_neo->cpuClockDivide == 2 || g_neo->cpuClockDivide == 3);

	guiConsoleLogf("Loading %s...", szFileName);
	guiConsoleDump();

	rom = systemOpen(szFileName, false);

	ASSERTMSG(g_neo->cpuClockDivide == 2 || g_neo->cpuClockDivide == 3, "%d", g_neo->cpuClockDivide);

	if(rom < 0) {
		guiConsoleLogf(" -> Failed!");
		ASSERT(0);
		return false;
	}
	systemRead(rom, &header, sizeof(TNeoRomHeader));

	if(header.magic != NEO_ROM_MAGIC ||
		header.version != NEO_ROM_VERSION ||
		header.sectionCount != NEOROM_COUNT) {

		systemClose(rom);
		guiConsoleLogf(" -> invalid rom");
		guiConsoleLogf(" -> magic %08X / %08X", header.magic, NEO_ROM_MAGIC);
		guiConsoleLogf(" -> version %d / %d", header.version, NEO_ROM_VERSION);
		guiConsoleLogf(" -> section %d / %d", header.sectionCount, NEOROM_COUNT);
		ASSERT(0);
		return false;
	}

	neoSystemClose();

	g_romSize = systemFileSize(rom) - NEO_HEADER_SIZE;
	g_header = header;
	g_rom = rom;

	linearHeapReset(&g_vramHHeap);
	linearHeapClear(&g_vramHHeap);

	g_neo->scanline = 0;
	g_neo->frameCount = 0;
	g_neo->irqPending = 0;
	g_neo->paletteBank = 0;
	g_neo->fixedBank = 0;
	g_neo->sramProtectCount = 0;
	g_neo->debug = true;

	g_neo->irqVectorLatch = false;
	g_neo->screenDarkLatch = false;
	g_neo->fixedRomLatch = false;
	g_neo->sramProtectLatch = false;
	g_neo->paletteRamLatch = false;
	g_neo->smaRand = 0x2345;
	NEOIPC->audioCommandPending = 0;
	NEOIPC->audioResult = 0;
	
	//sram hack gets around watchdog protection check...values taken from GnGeo
	//added samsho5 variations
	g_neo->sramProtection = g_header.sramProtection;

	guiConsoleLogf("Loaded Game: %s", g_header.name);
	
	g_neo->spriteCount = g_header.romEntry[NEOROM_SPRITEDATA].size / SPRITE_SIZE;
	g_neo->spriteMask = 0xffffffff;
	for(bit = 0x80000000; bit != 0; bit >>= 1) {
		if((g_neo->spriteCount - 1) & bit) break;
		g_neo->spriteMask >>= 1;
	}

	g_neo->romBankCount = 0;
	const s32 bankSize = (s32)g_header.romEntry[NEOROM_MAINPROGRAM].size - 1*MB;
	if(bankSize > 0) {
		g_neo->romBankCount = bankSize / (1*MB);
		if(g_neo->romBankCount * 1*MB != bankSize) {
			g_neo->romBankCount++;
			guiConsoleLogf(" -> partial rom bank");
		}
	}

	guiConsoleLogf(" -> sprites: %d", g_neo->spriteCount);
	guiConsoleLogf(" -> mask: %08X", g_neo->spriteMask);
	guiConsoleLogf(" -> rom bank(s): %d", g_neo->romBankCount);
	if(g_neo->sramProtection == -1) {
		guiConsoleLogf(" -> sramProtection off");
	} else {
		guiConsoleLogf(" -> sramProtection: %08X", g_neo->sramProtection);
	}
	guiConsoleDump();

	systemRamReset();
	systemSlot2Reset();

	cpuInit();
	neoMemoryInit();
	neoIOInit();
	pd4990a_init();
	neoVideoInit();
	neoInstallProtection();
	//neoSystemIrq(INTR_COLDBOOT);
	if(NEOIPC->globalAudioEnabled) {
		neoAudioStreamInit();
		neoIPCSendCommand(NEOARM7_RESET);
	} else {
		//this is the value that needs to be set to bypass Z80 check
		NEOIPC->audioResult = 1;
	}
	cpuReset();

	neoLoadConfig(szFileName);
	neoResetContext();

	ASSERTMSG(g_neo->cpuClockDivide == 2 || g_neo->cpuClockDivide == 3, "%d",
		g_neo->cpuClockDivide);
	
	return true;
}

void neoSystemClose()
{
	if(g_rom >= 0) {
		systemClose(g_rom);
		neoResetContext();
		g_rom = -1;
		neoSaveConfig();
	}
}

static inline void neoSystemUpdateIrq()
{
	if(g_neo->irqPending & INTR_COLDBOOT) {
		cpuInterrupt(3);
	} else if(g_neo->irqPending & INTR_DISPLAYPOS) {
		systemWriteLine("IRQ2");
		cpuInterrupt(2);
	} else if(g_neo->irqPending & INTR_VBLANK) {
		cpuInterrupt(1);
	} else {
		cpuInterrupt(0);
	}
}

void neoSystemIrqAk(u16 data)
{
	if(data & 0x01) {
		g_neo->irqPending &= ~INTR_COLDBOOT;
	}
	if(data & 0x02) {
		systemWriteLine("IRQ2 AK");
		g_neo->irqPending &= ~INTR_DISPLAYPOS;
	}
	if(data & 0x04) {
		g_neo->irqPending &= ~INTR_VBLANK;
	}
	neoSystemUpdateIrq();
}

void neoSystemIrq(u32 irq)
{
	g_neo->irqPending |= irq;
	neoSystemUpdateIrq();
}

void neoSystemReset()
{
	systemWriteLine("System reset");

	cpuReset();

	if(g_neo->irqVectorLatch) {
		g_neo->irqVectorLatch = false;
		neoSystemLoadRegion(NEOROM_BIOS, g_neo->pRom0, 128);
	}
	
	g_neo->screenDarkLatch = false;
	g_neo->sramProtectLatch = false;
	g_neo->paletteRamLatch = false;
	g_neo->paletteBank = 0;
	g_neo->watchdogCounter = 0;
}

void neoSystemSetEnabled(bool enable)
{
	if(enable && g_pauseCount == 1) {
		//unpausing
		g_neo->active = true;
		if(NEOIPC->globalAudioEnabled) {
			//only turn on arm7 if audio is on
			neoIPCSendCommand(NEOARM7_RESUME);
		}
	} else if(!enable && g_pauseCount == 0) {
		//pausing
		g_neo->active = false;
		if(NEOIPC->globalAudioEnabled) {
			//only turn on arm7 if audio is on
			neoIPCSendCommand(NEOARM7_PAUSE);
		}
	}

	if(enable) g_pauseCount--;
	else g_pauseCount++;
	systemWriteLine("Pause Count: %d", g_pauseCount);

	/*if(enable != g_neo->active) {
		g_neo->active = enable;
		if(NEOIPC->globalAudioEnabled) {
			//only turn on arm7 if audio is on
			if(enable) neoIPCSendCommand(NEOARM7_RESUME);
			else neoIPCSendCommand(NEOARM7_PAUSE);
		}
	}*/
}

//pressed is true if key was pressed just this frame
static void neoDoInput(u32 mask, bool pressed)
{
	u32 ctrl1Reg = g_neo->ctrl1Reg;
	u32 ctrl3Reg = g_neo->ctrl3Reg;
	u32 coinReg = g_neo->coinReg;
	if(g_neo->active) {
		if(mask & NEOINPUT_A) ctrl1Reg &= ~(1 << 12); //A
		if(mask & NEOINPUT_B) ctrl1Reg &= ~(1 << 13); //B
		if(mask & NEOINPUT_C) ctrl1Reg &= ~(1 << 14); //C
		if(mask & NEOINPUT_D) ctrl1Reg &= ~(1 << 15); //D
		if(mask & NEOINPUT_START) ctrl3Reg &= ~(1 << 8); //Start
		if(mask & NEOINPUT_SELECT) ctrl3Reg &= ~(1 << 9); //select
		if(mask & NEOINPUT_COIN) coinReg &= ~(1 << 0); //coin
	}
	//this is the only input that works when emulation is disabled
	//also, this only works when key is pressed (not held)
	if((mask & NEOINPUT_PAUSE) && pressed) {
		neoSystemSetEnabled(g_paused);
		g_paused = !g_paused;
	}

	g_neo->ctrl1Reg = ctrl1Reg;
	g_neo->ctrl3Reg = ctrl3Reg;
	g_neo->coinReg = coinReg;
}

static void neoSystemLidClose()
{
	if(g_pauseCount <= 0) {
		//if we are not paused, stop arm7
		neoIPCSendCommand(NEOARM7_PAUSE);
	}
	neoIPCSendCommand(NEOARM7_LIDCLOSE);
	while(1) {
		scanKeys();
		const u32 keys = keysHeld();
		if(!(keys & KEY_LID)) {
			break;
		}
		swiWaitForVBlank();
	}
	neoIPCSendCommand(NEOARM7_LIDOPEN);
	if(g_pauseCount <= 0) {
		//if we are not paused, resume arm7
		neoIPCSendCommand(NEOARM7_RESUME);
	}
	neoAudioStreamReset();
}

static void neoSystemDoKeys(u32 keys)
{
	static const u32 keyMask[8] = {
		KEY_A,
		KEY_B,
		KEY_X,
		KEY_Y,
		KEY_L,
		KEY_R,
		KEY_START,
		KEY_SELECT,
	};
	static u32 lastKeys = 0;
	const u32 keysChanged = keys ^ lastKeys;
	const u32 keysPressed = keys & keysChanged;
	s32 i;

	for(i = 7; i >=0; i--) {
		if(keys & keyMask[i]) {
			neoDoInput(g_neo->keyGrid[i], (keysPressed & keyMask[i]) != 0);
		}
	}

	if(keys & KEY_LID) {
		neoSystemLidClose();
	}

	lastKeys = keys;
}

static void neoSystemDoFrame()
{
	u32 keys;
	u32 input;

	ASSERT(g_neo->cpuClockDivide == 2 || g_neo->cpuClockDivide == 3);

	profilerPush(NEOPROFILE_CPU);

	g_neo->scanline = 0;
	cpuExecute(CPU_CLOCKS_PER_SCANLINE);

	profilerPop();

	//do frame
	g_neo->frameCount++;
	pd4990a_addretrace();
	neoAudioStreamProcess();

	g_neo->watchdogCounter++;
	/*if(g_neo->watchdogCounter > 10) {
		systemWriteLine("WATCHDOG!!!!");
		neoSystemReset();
		return;
	}*/


	keys = keysHeld();
	
	//arrow keys are not configurable
	input = 0xff;
	if(keys & KEY_UP) input &= ~(1 << 0);
	if(keys & KEY_DOWN) input &= ~(1 << 1);
	if(keys & KEY_LEFT) input &= ~(1 << 2);
	if(keys & KEY_RIGHT) input &= ~(1 << 3);
	g_neo->ctrl1Reg = 0x00ff | ((u16)input << 8);
	g_neo->ctrl2Reg = 0xffff;

	//input = 0x05;
	//if(keys & KEY_START) input &= ~(1 << 0);
	//mask with 0x7000 to disable memory card
	g_neo->ctrl3Reg = 0xcf00;//0x8A00 | 0x7000 | ((u16)input << 8);

	//input = 0x03;
	//if(keys & KEY_SELECT) input &= ~(1 << 0);
	//set audio command to 01 to hack out sound
	g_neo->coinReg = 0x0007 | //0x0004 | input |
		(read_4990_testbit() << 6) | (read_4990_databit() << 7);// | 
		//((u16)NEOIPC->audioResult << 8);
	/*if(!NEOIPC->globalAudioEnabled) {
		//hack audio ready bit
		g_neo->coinReg |= (1 << 8);
	}*/

	g_neo->ctrl4Reg = 0xff80;

	neoSystemDoKeys(keys);
}

void neoSystemExecute()
{
	profilerPush(NEOPROFILE_ROOT);
	while(1) {
		guiSystemProcess();
		if(g_neo->active) {
			neoSystemDoFrame();
		} else {
			//process input while paused
			const u32 keys = keysHeld();
			neoSystemDoKeys(keys);
			//and audio
			neoAudioStreamProcess();
			//wait for vblank
			swiWaitForVBlank();
			//and render the gui
			guiSystemRender();
		}
	}
	profilerPop();
}

	
		
