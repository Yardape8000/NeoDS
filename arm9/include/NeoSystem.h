#ifndef _NEO_SYSTEM_H
#define _NEO_SYSTEM_H

//#define NEO_AUDIOROM_IN_VRAM

#define NEO_ROM_MAGIC 0x7e0116e0
#define NEO_ROM_VERSION 0

//2 is normal
//3 or 4 to underclock
#define CPU_CLOCK_DIVIDE (g_neo->cpuClockDivide)
//#define CPU_CLOCK_DIVIDE 2

#define PIXEL_CLOCK (MAIN_CLOCK / PIXEL_CLOCK_DIVIDE)
#define CPU_CLOCK (MAIN_CLOCK / CPU_CLOCK_DIVIDE)

#define PIXELS_PER_CLOCK (PIXEL_CLOCK_DIVIDE / CPU_CLOCK_DIVIDE)
#define CPU_CLOCKS_PER_SCANLINE (PIXELS_PER_SCANLINE * PIXEL_CLOCK_DIVIDE / CPU_CLOCK_DIVIDE)
#define CPU_CLOCKS_PER_FRAME (CPU_CLOCKS_PER_SCANLINE * SCANLINES_PER_FRAME)
#define CPU_CLOCKS_PER_VBLANK (CPU_CLOCKS_PER_SCANLINE * SCANLINES_PER_VBLANK)
#define CPU_CLOCKS_PER_TOP (CPU_CLOCKS_PER_SCANLINE * SCANLINES_TOP)
#define CPU_CLOCKS_PER_BOTTOM (CPU_CLOCKS_PER_SCANLINE * SCANLINES_BOTTOM)
#define CPU_CLOCKS_PER_ACTIVE (CPU_CLOCKS_PER_SCANLINE * SCANLINES_ACTIVE)

//TNeoContext for asm
#define NEO_CPU 0
#define NEO_CPU_MEMBASE (NEO_CPU+0x60)

#define NEO_VIDEOWRITETABLE (NEO_CPU+0xb0)

#define NEO_SPRITECOUNT (NEO_VIDEOWRITETABLE+36)
#define NEO_ROMBANKS (NEO_SPRITECOUNT+4)

#define NEO_ADPCM (NEO_ROMBANKS+4)
#define NEO_ADPCMB (NEO_ADPCM + 12 * 7) //7 channels, 12 bytes per channel
#define NEO_ADPCMACTIVE (NEO_ADPCMB + 12) //1 channel, 12 bytes

#define NEO_ROM0 (NEO_ADPCMACTIVE+4)
#define NEO_ROM1 (NEO_ROM0+4)
#define NEO_BIOS (NEO_ROM1+4)
#define NEO_RAM (NEO_BIOS+4)
#define NEO_SRAM (NEO_RAM+4)
#define NEO_CARD (NEO_SRAM+4)
#define NEO_VRAM (NEO_CARD+4)
#define NEO_SPRITERAM (NEO_VRAM+4)
#define NEO_PALETTE (NEO_SPRITERAM+4)
//#define NEO_NITROPALETTE (NEO_PALETTE+4)
#define NEO_BANKTABLE (NEO_PALETTE+4)

#define NEO_SMAADD0 (NEO_BANKTABLE+4)
#define NEO_SMAADD1 (NEO_SMAADD0+4)
#define NEO_SMABANKADDR (NEO_SMAADD1+4)
#define NEO_SMABANKOFFSET (NEO_SMABANKADDR+4)
#define NEO_SMABANKBIT (NEO_SMABANKOFFSET+4)
#define NEO_SMARAND  (NEO_SMABANKBIT+4)
//#define NEO_BANKTABLEPTR (NEO_BANKTABLE + BANK_MAX * BANK_CACHE_COUNT * 4)

#define NEO_SCANLINE (NEO_SMARAND+4)
#define NEO_FRAMECOUNT (NEO_SCANLINE+4)
#define NEO_VRAMBASE (NEO_FRAMECOUNT+4)
#define NEO_VRAMMASK (NEO_VRAMBASE+4)
#define NEO_DISPLAYCOUNTER (NEO_VRAMMASK+4)
#define NEO_AUTOANIMCOUNTER (NEO_DISPLAYCOUNTER+4)
#define NEO_SPRITEMASK (NEO_AUTOANIMCOUNTER+4)
#define NEO_PALETTEDIRTY (NEO_SPRITEMASK+4)
#define NEO_TILEBUFFER (NEO_PALETTEDIRTY+64)

#define NEO_TILEOFFSETX (NEO_TILEBUFFER+4)
#define NEO_TILEOFFSETY (NEO_TILEOFFSETX+4)
#define NEO_TILESCALEX (NEO_TILEOFFSETY+4)
#define NEO_TILESCALEY (NEO_TILESCALEX+2)

#define NEO_CPUCLOCKDIVIDE (NEO_TILESCALEY+2)
#define NEO_CPUCLOCKSPERSCANLINE (NEO_CPUCLOCKDIVIDE+4)
#define NEO_IRQPENDING (NEO_CPUCLOCKSPERSCANLINE+4)
#define NEO_WATCHDOG (NEO_IRQPENDING+4)
#define NEO_PALETTEBANK (NEO_WATCHDOG+4)
#define NEO_FIXEDBANK (NEO_PALETTEBANK+4)
#define NEO_ROMBANK (NEO_FIXEDBANK+4)

#define NEO_SRAMPROTECTCOUNT (NEO_ROMBANK+4)
#define NEO_SRAMPROTECT (NEO_SRAMPROTECTCOUNT+4)

#define NEO_VRAMOFFSET (NEO_SRAMPROTECT+4)
#define NEO_VRAMMOD (NEO_VRAMOFFSET+4)
#define NEO_DISPLAYCONTROL (NEO_VRAMMOD+4)
#define NEO_DISPLAYCONTROLMASK (NEO_DISPLAYCONTROL+4)
#define NEO_DISPLAYCOUNTERLOAD (NEO_DISPLAYCONTROLMASK+4)

#define NEO_CTRL1REG (NEO_DISPLAYCOUNTERLOAD+4)
#define NEO_CTRL2REG (NEO_CTRL1REG+4)
#define NEO_CTRL3REG (NEO_CTRL2REG+4)
#define NEO_CTRL4REG (NEO_CTRL3REG+4)
#define NEO_COINREG (NEO_CTRL4REG+4)

#define NEO_IRQLATCH (NEO_COINREG+4)
#define NEO_DARKLATCH (NEO_IRQLATCH+1)
#define NEO_FIXEDLATCH (NEO_DARKLATCH+1)
#define NEO_SRAMLATCH (NEO_FIXEDLATCH+1)
#define NEO_PALETTELATCH (NEO_SRAMLATCH+1)

#define NEO_ACTVIE (NEO_PALETTELATCH+1)
#define NEO_DEBUG (NEO_ACTVIE+1)

#define NEO_KEYGRID (NEO_DEBUG+1)
#define NEO_FRAMECOUNTER (NEO_KEYGRID+8)
#define NEO_VAREND (NEO_FRAMECOUNTER+1)
//#define NEO_P1INPUT (NEO_FIXEDPALETTEDIRTY+1)
//#define NEO_COININPUT (NEO_P1INPUT+1)
//#define NEO_STARTINPUT (NEO_COININPUT+1)

#define NEO_READ8TABLE (0x400)
#define NEO_READ16TABLE (NEO_READ8TABLE + 256*4)
#define NEO_READ32TABLE (NEO_READ16TABLE + 256*4)
#define NEO_WRITE8TABLE (NEO_READ32TABLE + 256*4)
#define NEO_WRITE16TABLE (NEO_WRITE8TABLE + 256*4)
#define NEO_WRITE32TABLE (NEO_WRITE16TABLE + 256*4)
#define NEO_CHECKPCTABLE (NEO_WRITE32TABLE + 256*4)

#define NEO_CONTEXTEND (NEO_CHECKPCTABLE + 256*4)
//END TNeoContext

#include "NeoSystemCommon.h"

#ifndef NEO_IN_ASM

//includes needed for structs that are in TNeoContext
#include "NeoCpu.h"
#include "NeoMemory.h"
#include "NeoAudioStream.h"

#define NEOINPUT_A (1 << 0)
#define NEOINPUT_B (1 << 1)
#define NEOINPUT_C (1 << 2)
#define NEOINPUT_D (1 << 3)
#define NEOINPUT_START (1 << 4)
#define NEOINPUT_SELECT (1 << 5)
#define NEOINPUT_COIN (1 << 6)
#define NEOINPUT_PAUSE (1 << 7)

typedef enum _TNeoRomProtection {
	NEOPROT_NONE,
	NEOPROT_PVC,
	NEOPROT_KOF2000,
	NEOPROT_MSLUG3,
	NEOPROT_GAROUO,
	NEOPROT_GAROU,
	NEOPROT_KOF99,
	NEOPROT_KOF98,
	NEOPROT_FATFURY2,
} TNeoRomProtection;

typedef enum _TNeoRomRegion {
	NEOROM_MAINPROGRAM = 0,
	NEOROM_BIOS,
	NEOROM_AUDIOPROGRAM,
	NEOROM_AUDIOBIOS,
	NEOROM_AUDODATA,
	NEOROM_SPRITEDATA,
	NEOROM_FIXEDDATA, //bios, program
	NEOROM_SPRITEUSAGE, //bitmask of non-empty sprites
	NEOROM_TILEUSAGE, //bitmask of non-empty tiles
	NEOROM_COUNT,
} TNeoRomRegion;

typedef struct _TNeoRomEntry {
	u32 offset;
	u32 size;
} TNeoRomEntry;

typedef struct _TNeoRomHeader {
	u32 magic;
	u32 version;
	u32 protection;
	u32 sramProtection;
	u32 fixedBankType;
	u32 audio2Offset;
	u32 sectionCount;
	char name[16];
	TNeoRomEntry romEntry[NEOROM_COUNT];
	u8 data[0];
} TNeoRomHeader;

typedef enum _TNeoInterrups {
	INTR_VBLANK = 1,
	INTR_DISPLAYPOS = 2,
	INTR_COLDBOOT = 4,
} TNeoInterrups;

typedef struct _TNeoContext {
	union {
		struct {
			TCycloneContext cpu;
			
			//table to speed up vram writes
			void* restrict pVideoWriteTable[9]; //one extra entry so 32bit write can wrap
			
			//info about rom
			u32 spriteCount;
			u32 romBankCount;

			//audio data
			TNeoADPCMStream adpcm[7];
			TNeoADPCMBStream adpcmb;
			u32 adpcmActive;

			//memory pointers
			u8* restrict pRom0;
			u8* restrict pRom1; //sometimes banked
			u8* restrict pBios;
			u8* restrict pRam;
			u8* restrict pSram;
			u8* restrict pCard;
			u16* restrict pVram;
			u16* restrict pSpriteRam;
			u16* restrict pPalette;
			//u16* restrict pNitroPalette;
			const u8** restrict bankTable;

			//sma random generator protection
			u32 smaAddr0;
			u32 smaAddr1;
			u32 smaBankAddr;
			const u32* restrict smaBankoffset;
			const u32* restrict smaBankbit;
			u32 smaRand;

			//vram data
			u32 scanline;
			u32 frameCount;
			u16* restrict pVramBase; //set to either vram or spriteMem
			u32 vramBaseMask; //set to either 0x7fff or 7ff
			//u32 animCounter;
			u32 displayCounter;
			u32 autoAnimationCounter;
			u32 spriteMask;
			u32 paletteDirty[16];
			u16* pTileBuffer;

			//video data
			s32 tileOffsetX;
			s32 tileOffsetY;
			s16 tileScaleX;
			s16 tileScaleY;
			
			//system data
			u32 cpuClockDivide;
			u32 cpuClocksPerScanline;
			u32 irqPending;
			u32 watchdogCounter;
			u32 paletteBank;
			u32 fixedBank;
			u32 romBankAddress;
			//u32 romBank;
			//u32 keys;

			//memory data
			u32 sramProtectCount;
			s32 sramProtection;

			//registers
			u32 vramOffset;
			//u16 vramReadBuffer;
			u32 vramMod;
			u32 displayControl;
			u32 displayControlMask;
			u32 displayCounterLoad;

			u32 ctrl1Reg;
			u32 ctrl2Reg;
			u32 ctrl3Reg;
			u32 ctrl4Reg;
			u32 coinReg;
			
			//system latch
			u8 irqVectorLatch;
			u8 screenDarkLatch;
			u8 fixedRomLatch;
			u8 sramProtectLatch;
			u8 paletteRamLatch;

			//flags
			u8 active;
			u8 debug;

			//config
			u8 keyGrid[8];

			//etc
			u8 frameCounter;
			u8 varEnd[0];
		};
		u32 fill[0x400 / 4];
	};
	//TNeoMemoryTableEntry cpuMemTable[256];
	TRead8Func cpuRead8Table[256];
	TRead16Func cpuRead16Table[256];
	TRead32Func cpuRead32Table[256];
	TWrite8Func cpuWrite8Table[256];
	TWrite16Func cpuWrite16Table[256];
	TWrite32Func cpuWrite32Table[256];
	TCheckPcFunc cpuCheckPcTable[256];
} TNeoContext;

bool neoSystemInit();
bool neoSystemOpen(const char* szFileName);
void neoSystemReset();
void neoSystemIPCSync();
void neoSystemLoadRegionEx(TNeoRomRegion region, void* pDst, u32 offset, u32 maxSize);
void neoSystemLoadRegion(TNeoRomRegion region, void* pDst, u32 maxSize);
void neoSystemReadRegion(TNeoRomRegion region, void* pDst, u32 offset, u32 size);
void neoSystemReadRegionAsync(TNeoRomRegion region, void* pDst, u32 offset, u32 size);
u32 neoSystemRegionSize(TNeoRomRegion region);
void neoSystemClose();
void neoSystemExecute();
void neoSystemLoadSprite(u8* pDst, u32 index);
void neoSystemLoadSprite2(u8* pDst, u32 index);
void neoSystemLoadTile(u8* pDst, u32 index);
void neoSystemSetEnabled(bool enable);
void* neoSystemVramHAlloc(u32 size);
void neoSystemSetClockDivide(u32 clock);

u32 neoSystemGetRomCount();
const char* neoSystemGetRomName(u32 i);

void neoSystemIrqAk(u16 data);
void neoSystemIrq(u32 irq);

register TNeoContext* g_neo asm("r7");

#define NEO_SYSTEM_SECTION DTCM_BSS
//#define NEO_SYSTEM_SECTION 
extern TNeoContext g_neoContext NEO_SYSTEM_SECTION;
extern TNeoRomHeader g_header;

static inline void neoClearContext() { g_neo = 0; }
static inline void neoResetContext() { g_neo = &g_neoContext; }

#endif //NEO_IN_ASM

#endif
