#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoVideo.h"
#include "NeoMemory.h"
#include "NeoCpu.h"
#include <stdio.h>

typedef struct _TTileCacheEntry {
	u16 index;
	u16 frame;
} TTileCacheEntry;

#define TILE_NUMBER ((512+128)*2*KB / TILE_SIZE)

#define TILE_CACHE_VRAM ((u8*)0x06000000 + TILE_CACHE_ENTRY_SIZE)

#define TILE_CACHE_SIZE (56*KB / TILE_CACHE_ENTRY_SIZE - 1)
#define TILE_TABLE_SIZE ((512+128)*2*KB / TILE_CACHE_ENTRY_SIZE)

static TTileCacheEntry g_tileCache[TILE_CACHE_SIZE] DTCM_BSS;
static u32 g_cacheIndex = 0;

static u16 g_tileTable[TILE_TABLE_SIZE] ALIGN(32);
static u8 g_tileUsed[TILE_NUMBER / 8] ALIGN(32);
//static u16 g_tileMap[32*24] ALIGN(32);

static u8 g_tileLoadBuffer[TILE_MAX_LOAD * TILE_CACHE_ENTRY_SIZE] ALIGN(32);
static void* g_tileLoadAddr[TILE_MAX_LOAD] ALIGN(32);
static u32 g_tileLoadIndex = 0;

void neoFixedInit()
{
	s32 i;

	//set up fixed display
	BG2_CR =
		BG_PRIORITY_0 |
		BG_TILE_BASE(0) |
		BG_MAP_BASE(28) |
		BG_RS_64x64;

	BG3_CR =
		BG_PRIORITY_1 |
		BG_TILE_BASE(0) |
		BG_MAP_BASE(28) |
		BG_RS_64x64;

	//blend 2 backgrounds together (3 has lower priority than 2)
	BLEND_CR = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
	BLEND_AB = (8 << 0) | (8 << 8);

	g_neo->pTileBuffer = (u16*)neoSystemVramHAlloc(64 * 32 * sizeof(u16));

	for(i = 0; i < TILE_TABLE_SIZE; i++) {
		g_tileTable[i] = CACHE_ENTRY_NULL;
	}
	
	//load initial tile cache
	for(i = 0; i < TILE_CACHE_SIZE; i++) {
		TTileCacheEntry* pEntry = &g_tileCache[i];
		pEntry->index = i;
		pEntry->frame = 0xffff;

		neoSystemLoadTile(g_tileLoadBuffer, i);
		memcpy((u8*)TILE_CACHE_VRAM + i * TILE_CACHE_ENTRY_SIZE, g_tileLoadBuffer, TILE_CACHE_ENTRY_SIZE);
		g_tileTable[i] = i;
	}

	memset(g_tileUsed, 0, TILE_NUMBER / 8);
	neoSystemLoadRegion(NEOROM_TILEUSAGE, g_tileUsed, TILE_NUMBER / 8);

	switch(g_header.fixedBankType) {
	case 0: systemWriteLine("FIXED bank type 0"); break;
	case 1: systemWriteLine("FIXED bank type 1"); break;
	case 2: systemWriteLine("FIXED bank type 2"); break;
	default: systemWriteLine("FIXED bank type ??? (%d)", g_header.fixedBankType); break;
	}
}

void neoFixedExit()
{

}

void neoDrawFixed()
{
	//static u32 tileLoadIndex[TILE_MAX_LOAD];
	const s32 minX = g_videoBounds.minX >> 3;
	const s32 maxX = (g_videoBounds.maxX + 7) >> 3;
	const s32 minY = g_videoBounds.minY >> (3 + PIXEL_Y_SHIFT);
	const s32 maxY = (g_videoBounds.maxY + 7) >> (3 + PIXEL_Y_SHIFT);
	const s32 width = g_videoBounds.maxX - g_videoBounds.minX + 1;
	const s32 height = (g_videoBounds.maxY - g_videoBounds.minY + 1) >> PIXEL_Y_SHIFT;

	static u16 garouoffsets[32];
	//u32 loadIndex = 0;
	s32 mapX;
	s32 mapY;
	u32 bankType = g_header.fixedBankType;
	u32 tilesUsed = 0;

	g_tileLoadIndex = 0;

	if(g_neo->fixedBank == 0) {
		//no bank when bios tiles are loaded
		bankType = 0;
	}

	//setup scaling matrix
	g_neo->tileOffsetX = g_videoBounds.minX << 8;
	g_neo->tileOffsetY = g_videoBounds.minY << (8 - PIXEL_Y_SHIFT);
	g_neo->tileScaleX = (width << 8) / 256;
	g_neo->tileScaleY = (height << 8) / 192;

	if(bankType == 1) {
		u32 garoubank = 0;
		u32 k = 0;
		u32 y = 0;
		while(y < 32) {
			if(g_neo->pVram[0x7500 + k] == 0x0200 &&
				(g_neo->pVram[0x7580 + k] & 0xff00) == 0xff00) {
				garoubank = g_neo->pVram[0x7580 + k] & 3;
				garouoffsets[y++] = garoubank;
			}
			if(y >= 32) break; //added safety check
			garouoffsets[y++] = garoubank;
			k += 2;
		}
	}

	ASSERTMSG(maxX < 64, "x overflow: %d, (%d x %d)", maxX, g_videoBounds.maxX, g_videoBounds.maxY);
	ASSERTMSG(maxY < 32, "y overflow: %d, (%d x %d)", maxY, g_videoBounds.maxX, g_videoBounds.maxY);
	
	//figure out what tiles need to be loaded
	for(mapX = minX; mapX <= maxX; mapX++) {
		const u16* restrict pMapSrc = &g_neo->pVram[0x7000 + mapX * 32 + minY];
		u16* restrict pDst = &g_neo->pTileBuffer[minY * 64 + mapX];
		for(mapY = minY; mapY <= maxY; mapY++) {
			const u16 index = *pMapSrc++;
			u32 tileIndex = (u32)(index & 0x0fff) + g_neo->fixedBank;

			ASSERTMSG(pDst <  &g_neo->pTileBuffer[64 * 32], "buffer overflow");

			if(bankType == 1) {
				u32 offset = (mapY - 2) & 31;
				tileIndex += 0x1000 * (garouoffsets[offset] ^ 3);
			} else if(bankType == 2) {
				u32 vramOffset = 0x7500 + ((mapY - 1) & 31) + 32 * ((mapX) / 6);
				u32 vramShift = 5 - ((mapX) % 6);
				tileIndex += 0x1000 * (((g_neo->pVram[vramOffset] >> vramShift * 2) & 3) ^ 3);
			}

			if(g_tileUsed[tileIndex >> 3] & (1 << (tileIndex & 7))) {
				const u32 palIndex = (index & 0xf000);
				const u32 cacheEntryIndex = tileIndex / TILES_PER_ENTRY;
				ASSERT(cacheEntryIndex < TILE_TABLE_SIZE);
				u32 cacheIndex = g_tileTable[cacheEntryIndex];
				
				//only try to load tiles if we haven't yet loaded TILE_MAX_LOAD
				if(cacheIndex == CACHE_ENTRY_NULL && tilesUsed < TILE_CACHE_SIZE &&
				g_tileLoadIndex < TILE_MAX_LOAD) {
					//tile is not in cache, must be loaded
					void* pLoadAddr = UNCACHED(&g_tileLoadBuffer[g_tileLoadIndex * TILE_CACHE_ENTRY_SIZE]);
					//don't unload tiles that were used this frame
					do {
						g_cacheIndex++;
						//TILE_CACHE_SIZE not power of 2
						if(g_cacheIndex >= TILE_CACHE_SIZE) g_cacheIndex = 0;
					} while(g_tileCache[g_cacheIndex].frame == g_frameCount);
					cacheIndex = g_cacheIndex;
					if(cacheIndex != CACHE_ENTRY_NULL) {
						//mark old tile as no longer in cache
						//g_tileCache[cacheIndex].index == old tile
						ASSERT(cacheIndex < TILE_CACHE_SIZE);
						ASSERT(g_tileCache[cacheIndex].index < TILE_TABLE_SIZE);
						g_tileTable[g_tileCache[cacheIndex].index] = CACHE_ENTRY_NULL;
						//mark new tile as in cache
						g_tileTable[cacheEntryIndex] = cacheIndex;
						g_tileCache[cacheIndex].index = cacheEntryIndex;
						//load tile into vram
						neoSystemLoadTile(pLoadAddr, cacheEntryIndex);
						g_tileLoadAddr[g_tileLoadIndex] =
							(u8*)TILE_CACHE_VRAM + cacheIndex * TILE_CACHE_ENTRY_SIZE;
						g_tileLoadIndex++;
					}
				}
				if(cacheIndex != CACHE_ENTRY_NULL) {
					ASSERT(cacheIndex < TILE_CACHE_SIZE);
					if(g_tileCache[cacheIndex].frame != g_frameCount) {
						g_tileCache[cacheIndex].frame = g_frameCount;
						tilesUsed++;
					}
					const u32 vramIndex = cacheIndex * TILES_PER_ENTRY +
						(tileIndex & (TILES_PER_ENTRY - 1)) + TILES_PER_ENTRY;
					*pDst = vramIndex | palIndex;
				} else {
					*pDst = 0;
				}
			} else {
				*pDst = 0;
			}

			pDst += 64;
		}
	}
}

void neoLoadTiles()
{
	const u8* restrict pSrc = g_tileLoadBuffer;
	u32 i;
	
	//dma new tiles into video memory
	for(i = 0; i < g_tileLoadIndex; i++) {
		dmaCopyWords(3, pSrc, g_tileLoadAddr[i], TILE_CACHE_ENTRY_SIZE);
		pSrc += TILE_CACHE_ENTRY_SIZE;
	}
	
	//dma tilemap buffer
	dmaCopyWords(3, g_neo->pTileBuffer, (void*)0x0600e000, 64*32*sizeof(u16));
}
