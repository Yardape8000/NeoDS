#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoVideo.h"
#include "NeoMemory.h"

#define SCALE_SHIFT 2

typedef struct _TSpritePacket {
	u32 cmd0;
	u32 texImage;
	u16 palette;
	u16 palettePad;
	u16 texCoordx;
	u16 texCoordy;

	u32 cmd1;
	u32 vertex0;
	u32 vertex1;
	u32 vertex2;
	u32 vertex3;
} TSpritePacket;

#define SPRITE_PACKET_MAX 32

typedef struct _TSpriteDisplayList {
	u32 cmd;
	u32 m[12];
	u32 begin;
	TSpritePacket packet[SPRITE_PACKET_MAX];
} TSpriteDisplayList;

#define LIST_HEADER_SIZE (14*4)

typedef struct _TSpriteCacheEntry {
	u16 index; //maps to entry in g_spriteTable
	u16 frame;
} TSpriteCacheEntry;

//copies pDst -> SPRITE_CACHE2_MEM(saveIndex), then pSrc -> pDst
typedef struct _TSpriteTransferEntry {
	void* pDst;
} TSpriteTransferEntry;

#define SPRITE_CACHE1_MEM(i) ((u8*)VRAM_A + i * SPRITE_CACHE_ENTRY_SIZE)
#define SPRITE_CACHE1_SIZE (512*KB)

#define SPRITE_CACHE2_MEM(i) (g_spriteCache2Ram + i * SPRITE_CACHE2_ENTRY_SIZE)

#define SPRITE_CACHE1_COUNT (SPRITE_CACHE1_SIZE / SPRITE_CACHE_ENTRY_SIZE)
#define SPRITE_CACHE_COUNT SPRITE_CACHE1_COUNT

#define SPRITE_CACHE1_FIRST 0
#define SPRITE_CACHE1_LAST (SPRITE_CACHE1_FIRST + SPRITE_CACHE1_COUNT - 1)

#define SPRITE_TABLE_SIZE (64*MB / SPRITE_CACHE_ENTRY_SIZE)
#define SPRITE_TABLE2_SIZE (64*MB / SPRITE_CACHE2_ENTRY_SIZE)
#define SPRITE_NUMBER (64*MB / SPRITE_SIZE)

static u8* g_spriteCache2Ram;
static TSpriteCacheEntry g_spriteCache[SPRITE_CACHE_COUNT] DTCM_BSS;
static TSpriteCacheEntry* g_spriteCache2;
static u32 g_spriteCache2Count;
static u32 g_cacheIndex DTCM_BSS;
static u32 g_cache2Index DTCM_BSS;
static u8* g_spriteReadBuffer; //read cache2 entries into here

//one entry in array per 1024 byte sprite entry in neogeo sprite ROM
//each entry here maps to g_spriteCache
static u16 g_spriteTable[SPRITE_TABLE_SIZE] ALIGN(32);
static u16* g_spriteTable2;
static u8 g_spriteUsed[SPRITE_NUMBER / 8] ALIGN(32);

static u8 g_spriteLoadBuffer[SPRITE_MAX_LOAD * SPRITE_CACHE_ENTRY_SIZE] ALIGN(512);
static TSpriteTransferEntry g_spriteTransfer[SPRITE_MAX_LOAD] ALIGN(32);
static u32 g_spriteTransferIndex ALIGN(32);
u32 g_spriteCount;

static TSpriteDisplayList* g_spriteList;

#define CMD_BEGIN 0x40
#define CMD_END 0x41
#define CMD_VERTEX_16 0x23
#define CMD_VERTEX_XY 0x25
#define CMD_TEXCOORD 0x22
#define CMD_TEXIMAGE_PARAM 0x2a
#define CMD_PLTT_BASE 0x2b
#define CMD_LOAD_MATRIX4x3 0x17

#define TEXCOORD_X0 (16<<4)
#define TEXCOORD_Y0 (16<<20)
#define TEXCOORD_X1 (32<<4)
#define TEXCOORD_Y1 (32<<20)

void neoSpriteInit()
{
	s32 i;

	g_spriteList = (TSpriteDisplayList*)neoSystemVramHAlloc(sizeof(TSpriteDisplayList));

	g_spriteList->cmd = (CMD_LOAD_MATRIX4x3 << 0) | (CMD_BEGIN << 8);
	g_spriteList->m[0] = 1 << (12 - SCALE_SHIFT);
	g_spriteList->m[1] = 0;
	g_spriteList->m[2] = 0;

	g_spriteList->m[3] = 0;
	g_spriteList->m[4] = 1 << (12 - SCALE_SHIFT);
	g_spriteList->m[5] = 0;

	g_spriteList->m[6] = 0;
	g_spriteList->m[7] = 0;
	g_spriteList->m[8] = 1 << (12 - SCALE_SHIFT);

	g_spriteList->m[9] = 0;
	g_spriteList->m[10] = 0;
	g_spriteList->m[11] = 0;

	g_spriteList->begin = 1;

	const s32 x0 = 0;
	const s32 x1 = (1 << SCALE_SHIFT);
	for(i = 0; i < SPRITE_PACKET_MAX; i++) {
		const s32 y0 = i << (16 + SCALE_SHIFT);
		const s32 y1 = (i + 1) << (16 + SCALE_SHIFT);

		g_spriteList->packet[i].cmd0 = (CMD_TEXIMAGE_PARAM << 0) | (CMD_PLTT_BASE << 8) |
			(CMD_TEXCOORD << 16);// | (CMD_BEGIN << 24);
		g_spriteList->packet[i].cmd1 = (CMD_VERTEX_XY << 0) | (CMD_VERTEX_XY << 8) |
			(CMD_VERTEX_XY << 16) | (CMD_VERTEX_XY << 24);

		g_spriteList->packet[i].texCoordx = 16<<4;
		//if index is even, we must shift texCoord by 16 pixels
		g_spriteList->packet[i].texCoordy = (~i & 1) << 8;
		//pList->packet[i].vertex0z = 0;
		g_spriteList->packet[i].vertex0 = x0 | y0;
		g_spriteList->packet[i].vertex1 = x0 | y1;
		g_spriteList->packet[i].vertex2 = x1 | y1;
		g_spriteList->packet[i].vertex3 = x1 | y0;
	}

	vramSetBankA(VRAM_A_TEXTURE_SLOT0);
	vramSetBankB(VRAM_B_TEXTURE_SLOT1);
	vramSetBankC(VRAM_C_TEXTURE_SLOT2);
	vramSetBankD(VRAM_D_TEXTURE_SLOT3);

	for(i = 0; i < SPRITE_TABLE_SIZE; i++) {
		g_spriteTable[i] = CACHE_ENTRY_NULL;
	}

	g_frameCount = 0;

	//allocate texture memory to cpu
	u32 banks = vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	
	//load initial sprite cache
	for(i = 0; i < SPRITE_CACHE_COUNT; i++) {
		TSpriteCacheEntry* pEntry = &g_spriteCache[i];
		pEntry->index = i;

		neoSystemLoadSprite(g_spriteLoadBuffer, i);

		DC_FlushRange(g_spriteLoadBuffer, SPRITE_CACHE_ENTRY_SIZE);
		dmaCopyWords(3, g_spriteLoadBuffer, SPRITE_CACHE1_MEM(i), SPRITE_CACHE_ENTRY_SIZE);
		//memcpy(SPRITE_CACHE1_MEM(i), g_spriteLoadBuffer, SPRITE_CACHE_ENTRY_SIZE);

		g_spriteTable[i] = i;
	}

	//memcpy(g_spriteLoadBuffer, SPRITE_CACHE1_MEM, SPRITE_MAX_LOAD * SPRITE_CACHE_ENTRY_SIZE);

	//give texture memory back to gpu
	vramRestoreMainBanks(banks);

	memset(g_spriteUsed, 0, SPRITE_NUMBER / 8);
	neoSystemLoadRegion(NEOROM_SPRITEUSAGE, g_spriteUsed, SPRITE_NUMBER / 8);

	const u32 slot2Free = systemGetSlot2Free();
	u32 cache2Size = 0;

	if(slot2Free >= 20*MB) {
		cache2Size = 16*MB;
	} else if(slot2Free >= 10*MB) {
		cache2Size = 8*MB;
	} else if(slot2Free >= 5*MB) {
		cache2Size = 4*MB;
	}

	if(cache2Size > 0) {
		//set up secondary cache
		systemSlot2Unlock();
		g_spriteCache2Count = cache2Size / SPRITE_CACHE2_ENTRY_SIZE;
		g_spriteCache2Ram = systemSlot2Alloc(g_spriteCache2Count * SPRITE_CACHE2_ENTRY_SIZE);
		g_spriteCache2 = (TSpriteCacheEntry*)systemSlot2Alloc(sizeof(TSpriteCacheEntry) * g_spriteCache2Count);
		g_spriteTable2 = systemSlot2Alloc(SPRITE_TABLE2_SIZE * sizeof(u16));

		for(i = 0; i < SPRITE_TABLE2_SIZE; i++) {
			g_spriteTable2[i] = CACHE_ENTRY_NULL;
		}
		for(i = 0; i < g_spriteCache2Count; i++) {
			TSpriteCacheEntry* pEntry = &g_spriteCache2[i];
			pEntry->frame = 0;
			pEntry->index = CACHE_ENTRY_NULL;
		}
		g_cache2Index = 0;
		g_spriteReadBuffer = systemRamAlloc(SPRITE_CACHE2_ENTRY_SIZE);
		systemWriteLine("Sprite cache2 size: %d", cache2Size);
		systemSlot2Lock();
	} else {
		g_spriteCache2Count = 0;
		g_spriteTable2 = 0;
		g_spriteCache2Ram = 0;
		g_spriteTable2 = 0;
		g_spriteReadBuffer = 0;
		systemWriteLine("No sprite cache2");
	}
	//set up initial texture matrix
	MATRIX_CONTROL = 3;
	//scale up by 16 in x and y direction
	MATRIX_LOAD4x3 = 16 << (4 + 12 + 12 - SCALE_SHIFT);
	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 0;

	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 16 << (4 + 12 + 12 - SCALE_SHIFT);
	MATRIX_LOAD4x3 = 0;

	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 0;

	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 0;
	MATRIX_LOAD4x3 = 0;

	MATRIX_CONTROL = 1;
}

void neoSpriteExit()
{

}

static inline u32 getSpriteIndex(u32 tileControl)
{
	u32 index = (u16)tileControl | ((tileControl & 0x00700000) >> 4);
	if(!(g_neo->displayControl & 0x08)) {
		//auto animation is on
		if(tileControl & 0x080000) {
			index = (index & ~0x07) | (g_neo->autoAnimationCounter & 0x07);
		} else if((tileControl & 0x040000) != 0) {
			index = (index & ~0x03) | (g_neo->autoAnimationCounter & 0x03);
		}
	}
	return index & g_neo->spriteMask;
}

static inline void drawSpriteTile(TSpritePacket* pPacket,
								  u32 tileIndex, u32 tileControl)
{
	pPacket->texImage = (tileIndex << 4) | //vram offset
		((tileControl & 0x030000) << 2) | //flip
		(1 << 16) | //repeat x
		(1 << 17) | //repeat y
		(1 << 20) | //width
		(1 << 23) | //height
		(3 << 26) | //format
		(1 << 29) | //trans color0
		(3 << 30); //position texgen
	//set palette, include palette bank
	pPacket->palette = ((tileControl >> 23) & 0x1fe) +
		((u32)g_neo->paletteRamLatch << 9);
}

void neoDrawSprites()
{
	TSpriteDisplayList* const restrict pDisplayList = g_spriteList;
	u32 lastCache2EntryIndex = -1;
	u32 zoomControl;
	s32 zoomY = 0;
	s32 fxXPos = 0;
	s32 fxYPos = 0;
	s32 fxWidth = 1;
	s32 fxHeight = 1;
	s32 fxYPosClamp = (0x200 << PIXEL_Y_SHIFT);
	s32 rows = 0;
	u32 fullmode = 0; //named by gngeo
	u8* pLoadDst = g_spriteLoadBuffer;
	u32 spritesUsed = 0;
	u32 i;

	g_spriteTransferIndex = 0;

	//prep dma
	DMA_SRC(3) = (u32)pDisplayList;
	DMA_DEST(3) = (u32)&GFX_FIFO;

	for(i = 0; i < 381; i++) {
		const u32 yControl = (u32)g_neo->pSpriteRam[0x200 | i];

		if(yControl & 0x40) {
			//chained
			if(rows == 0) {
				continue;
			}
			zoomControl = (u32)g_neo->pSpriteRam[i];
			fxXPos = (fxXPos + fxWidth) & (0x200 - 1);
			fxWidth = ((zoomControl & 0x0f00) + 0x100) >> 8;
		} else {
			//new block
			rows = yControl & 0x3f;
			if(rows == 0) {
				//null sprite
				continue;
			}
			zoomControl = (u32)g_neo->pSpriteRam[i];
			fxXPos = (g_neo->pSpriteRam[0x400 | i] >> 7);
			fxYPos = (0x200 << PIXEL_Y_SHIFT) - ((s32)(yControl >> 7) << PIXEL_Y_SHIFT);
			zoomY = zoomControl & 0xff;
			fxWidth = ((zoomControl & 0x0f00) + 0x100) >> 8;
			fxHeight = (zoomY + 1) << (PIXEL_Y_SHIFT - 4);

			if(rows > 0x20) {
				//clamp rows to 32
				rows = 0x20;
				fullmode = 1;
				fxYPosClamp = (zoomY + 1) << (1 + PIXEL_Y_SHIFT);

				if(fxYPos + fxHeight >= (0x100 << PIXEL_Y_SHIFT)) {
					fxYPos -= (0x200 << PIXEL_Y_SHIFT);
				}
				while(fxYPos < (-16) << PIXEL_Y_SHIFT) {
					fxYPos += fxYPosClamp;
				}
			} else {
				fullmode = 0; //default
				fxYPosClamp = 0x200 << PIXEL_Y_SHIFT;
			}
		}

		if(fxXPos + fxWidth >= 0x200) {
			fxXPos -= 0x200;
		}
		
		//make sure sprite is on screen
		if(fxXPos + fxWidth < g_videoBounds.minX || fxXPos > g_videoBounds.maxX) {
			continue;
		}
		
		//prepare to draw sprite column
		const u32* restrict pTable = (u32*)&g_neo->pVram[i * 64];
		u32 packetCount = 0;
		s32 fxTileYPos = fxYPos;
		s32 fxHeightToDraw;
		bool matrixDirty = false;
		s32 y = 0;

		pDisplayList->m[0] = (fxWidth << (12 - SCALE_SHIFT)); //scale x
		pDisplayList->m[4] = (fxHeight << (12 - SCALE_SHIFT)); //scale y
		pDisplayList->m[9] = fxXPos; //trans x
		pDisplayList->m[10] = fxTileYPos; //trans y
		pDisplayList->m[11] = -(1 << 12) + 1 + i; //trans z

		if(g_videoBounds.maxX != (256 + 32 - 1)) {
			pDisplayList->m[0] += 32; //fudge
		}

		if(fullmode) {
			fxHeightToDraw = rows * fxHeight;
		} else {
			fxHeightToDraw = rows << (4 + PIXEL_Y_SHIFT);
		}

		while(fxHeightToDraw > 0) {
			const u32 tileControl = *pTable++;

			if(fxTileYPos >= (248 << PIXEL_Y_SHIFT)) {
				//position wrap
				matrixDirty = true;
				fxTileYPos -= fxYPosClamp;
			}
			
			if(fxTileYPos + fxHeight < g_videoBounds.minY || fxTileYPos > g_videoBounds.maxY) {
				//tile not on screen
				matrixDirty = true;
				goto skipTile;
			}
			const u32 actualIndex = getSpriteIndex(tileControl);
			if(!(g_spriteUsed[actualIndex >> 3] & (1 << (actualIndex & 7)))) {
				//tile is blank
				matrixDirty = true;
				goto skipTile;
			}

			const u32 cacheEntryIndex = actualIndex >> SPRITES_PER_ENTRY_SHIFT;
			u32 cache1Index = (u32)g_spriteTable[cacheEntryIndex];
			ASSERT(cacheEntryIndex < SPRITE_TABLE_SIZE);

			if(cache1Index == CACHE_ENTRY_NULL && spritesUsed < SPRITE_CACHE_COUNT &&
			g_spriteTransferIndex < SPRITE_MAX_LOAD) {
				//sprite is not in cache, must be loaded
				TSpriteTransferEntry* pTransfer = &g_spriteTransfer[g_spriteTransferIndex++];
				
				//don't unload sprites that were used this frame
				do {
					g_cacheIndex = (g_cacheIndex + 1) & (SPRITE_CACHE_COUNT - 1);
				} while(g_spriteCache[g_cacheIndex].frame == g_frameCount);
				cache1Index = g_cacheIndex;
				
				//evict old sprite from cache1
				ASSERT(cache1Index < SPRITE_CACHE_COUNT);
				//ASSERT(g_spriteCache[cache1Index].index < SPRITE_TABLE_SIZE);
				g_spriteTable[g_spriteCache[cache1Index].index] = CACHE_ENTRY_NULL;
				//mark new sprite as in cache
				g_spriteTable[cacheEntryIndex] = cache1Index;
				g_spriteCache[cache1Index].index = cacheEntryIndex;

				//wait for dma to finish before entering dldi driver
				while(DMA_SRC(3) & DMA_BUSY) continue;

				if(g_spriteTable2) {
					const u32 cache2EntryIndex = actualIndex >> SPRITES_PER_ENTRY_SHIFT2;
					const u32 cache2Offset =
						(actualIndex & (SPRITES_PER_ENTRY2 - 1)) >> SPRITES_PER_ENTRY_SHIFT;
					u32 cache2Index;

					if(lastCache2EntryIndex == cache2EntryIndex) {
						DMA_SRC(3) = (u32)g_spriteReadBuffer + cache2Offset * SPRITE_CACHE_ENTRY_SIZE;
						DMA_DEST(3) = (u32)pLoadDst;
						DMA_CR(3) = DMA_COPY_WORDS | (SPRITE_CACHE_ENTRY_SIZE >> 2);
					} else {
						//g_spriteTable2 is in slot2...unlock first
						systemSlot2Unlock();
						cache2Index = g_spriteTable2[cache2EntryIndex];
						if(cache2Index == CACHE_ENTRY_NULL) {
							g_cache2Index++;
							if(g_cache2Index >= g_spriteCache2Count) {
								g_cache2Index = 0;
							}
							cache2Index = g_cache2Index;

							ASSERT(cache2Index < g_spriteCache2Count);
							const u32 oldCache2EntryIndex = g_spriteCache2[cache2Index].index;
							if(oldCache2EntryIndex != CACHE_ENTRY_NULL) {
								ASSERTMSG(oldCache2EntryIndex < SPRITE_TABLE2_SIZE, "%d/%d",
									oldCache2EntryIndex, SPRITE_TABLE2_SIZE);
								g_spriteTable2[oldCache2EntryIndex] = CACHE_ENTRY_NULL;
							}
							//mark new sprite as in cache
							ASSERT(cache2EntryIndex < SPRITE_TABLE2_SIZE);
							g_spriteTable2[cache2EntryIndex] = cache2Index;
							g_spriteCache2[cache2Index].index = cache2EntryIndex;

							//lock slot2 before dldi
							systemSlot2Lock();
							//load into ram
							neoSystemLoadSprite2(UNCACHED(g_spriteReadBuffer), cache2EntryIndex);
							//dma a copy into secondary cache in slot2
							systemSlot2Unlock();
							DMA_SRC(3) = (u32)g_spriteReadBuffer;
							DMA_DEST(3) = (u32)SPRITE_CACHE2_MEM(cache2Index);
							DMA_CR(3) = DMA_COPY_WORDS | (SPRITE_CACHE2_ENTRY_SIZE >> 2);
							volatile u32 dummy = DMA_CR(3);
							dummy = DMA_CR(3);
							while(DMA_SRC(3) & DMA_BUSY) continue;
							//and lock slot2 again
							systemSlot2Lock();
							//dma a copy into load buffer
							DMA_SRC(3) = (u32)g_spriteReadBuffer + cache2Offset * SPRITE_CACHE_ENTRY_SIZE;
							DMA_DEST(3) = (u32)pLoadDst;
							DMA_CR(3) = DMA_COPY_WORDS | (SPRITE_CACHE_ENTRY_SIZE >> 2);
							lastCache2EntryIndex = cache2EntryIndex;
						} else {
							//dma from secondary cache into load buffer
							DMA_SRC(3) = (u32)SPRITE_CACHE2_MEM(cache2Index) + cache2Offset * SPRITE_CACHE_ENTRY_SIZE;
							DMA_DEST(3) = (u32)pLoadDst;
							DMA_CR(3) = DMA_COPY_WORDS | (SPRITE_CACHE_ENTRY_SIZE >> 2);
							while(DMA_SRC(3) & DMA_BUSY) continue;
							//and lock slot2 again
							systemSlot2Lock();
						}
					}
				} else {
					//load into load buffer
					neoSystemLoadSprite(pLoadDst, cacheEntryIndex);
				}
				pLoadDst += SPRITE_CACHE_ENTRY_SIZE;
				pTransfer->pDst = SPRITE_CACHE1_MEM(cache1Index);
				//restore dma
				DMA_SRC(3) = (u32)pDisplayList;
				DMA_DEST(3) = (u32)&GFX_FIFO;
			}

			if(cache1Index == CACHE_ENTRY_NULL) {
				//failed to load
				matrixDirty = true;
				goto skipTile;
			}

			ASSERT(cache1Index < SPRITE_CACHE_COUNT);
			if(matrixDirty || packetCount >= SPRITE_PACKET_MAX) {
				//flush existing sprites
				if(packetCount > 0) {
					//wait for fifo to be less than half full
					while(DMA_SRC(3) & DMA_BUSY) continue;
					const u32 listSize = (u32)&pDisplayList->packet[packetCount] - (u32)pDisplayList;
					DMA_CR(3) = DMA_FIFO | (listSize >> 2);
				}
				packetCount = 0;
				//prep matrix
				pDisplayList->m[10] = fxTileYPos; //trans y
				matrixDirty = false;
			}

			if(g_spriteCache[cache1Index].frame != g_frameCount) {
				spritesUsed++;
				g_spriteCache[cache1Index].frame = g_frameCount;
			}
			
			const u32 vramIndex =
				(cache1Index << SPRITES_PER_ENTRY_SHIFT) +
				(actualIndex & (SPRITES_PER_ENTRY - 1));
			drawSpriteTile(&pDisplayList->packet[packetCount], vramIndex, tileControl);
			packetCount++;
skipTile:
			fxTileYPos += fxHeight;
			fxHeightToDraw -= fxHeight;
			y++;

			if(fullmode == 0 && (y == 0x10 || y == 0x20)) {
				const s32 skip = (0xff - zoomY) << (1 + PIXEL_Y_SHIFT);
				fxTileYPos += skip;
				fxHeightToDraw -= skip;
				matrixDirty = true;
			}
		}

		if(packetCount > 0) {
			//wait for fifo to be less than half full
			while(DMA_SRC(3) & DMA_BUSY) continue;
			const u32 listSize = (u32)&pDisplayList->packet[packetCount] - (u32)pDisplayList;
			DMA_CR(3) = DMA_FIFO | (listSize >> 2);
		}
	}

	//wait for final dma to end
	while(DMA_SRC(3) & DMA_BUSY) continue;
	while(GFX_BUSY) continue;
	g_spriteCount = GFX_POLYGON_RAM_USAGE;
}

void neoLoadSprites()
{
	const u8* restrict pSrc = g_spriteLoadBuffer;
	const TSpriteTransferEntry* restrict pTransfer = g_spriteTransfer;
	volatile u32 dummy;
	s32 i;

	//allocate texture memory to cpu
	u32 banks = vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	
	//dma new sprites into texture memory
	for(i = g_spriteTransferIndex - 1; i >= 0 ; i--, pTransfer++) {
		DMA_SRC(3) = (u32)pSrc;
		DMA_DEST(3) = (u32)pTransfer->pDst;
		DMA_CR(3) = DMA_COPY_WORDS | (SPRITE_CACHE_ENTRY_SIZE >> 2);
		pSrc += SPRITE_CACHE_ENTRY_SIZE;
		//wait for dma to stat
		dummy = DMA_CR(3);
		dummy = DMA_CR(3);
		while(DMA_CR(3) & DMA_BUSY) continue;
	}
	
	//give texture memory back to gpu
	vramRestoreMainBanks(banks);
}
