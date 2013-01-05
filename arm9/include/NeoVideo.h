#ifndef _NEO_VIDEO_H
#define _NEO_VIDEO_H

#include "NeoIO.h"

typedef struct _TNeoVideoBounds {
	s32 minX;
	s32 minY;
	s32 maxX;
	s32 maxY;
} TNeoVideoBounds;

typedef enum _TNeoVideoSize {
	NEOVIDEO_NORMAL,
	NEOVIDEO_SCALED,
	NEOVIDEO_SIZECOUNT,
} TNeoVideoSize;

u16 neoVideoReadPal16(u32 a);
void neoVideoWritePal8(u32 a, u8 d);
void neoVideoWritePal16(u32 a, u16 d);
void neoVideoPaletteBank(u32 enable);

/*static inline void neoVideoWritePal32(u32 a, u32 d)
{
	neoVideoWritePal16(a, (u16)(d >> 16));
	neoVideoWritePal16(a + 2, (u16)d);
}*/

u16 neoVideoReadPal16(u32 a);
bool neoVideoInit();
void neoVideoDrawFrame();
void neoVideoFinishFrame();
bool neoVideoIsFramePending();
void neoVideoSetSize(TNeoVideoSize size);
TNeoVideoSize neoVideoGetSize();

#define FRAME_VCOUNT_CUTOFF 186

//u16 neoVideoReadPal16(u32 a);
//NEOIO_DEFINE_READ(neoVideoReadPal);

//#define FIXED_ROM_ADDR ((u8*)0x06000000)
//#define FIXED_ROM_SIZE (32*KB)

#define PIXEL_Y_SHIFT 4
//#define PIXEL_SHIFT 12

#define SPRITE_SIZE 128

#define SPRITES_PER_ENTRY_SHIFT 3
#define SPRITES_PER_ENTRY (1 << SPRITES_PER_ENTRY_SHIFT)
#define SPRITE_CACHE_ENTRY_SIZE (SPRITES_PER_ENTRY * SPRITE_SIZE)

#define SPRITES_PER_ENTRY_SHIFT2 5
#define SPRITES_PER_ENTRY2 (1 << SPRITES_PER_ENTRY_SHIFT2)
#define SPRITE_CACHE2_ENTRY_SIZE (SPRITES_PER_ENTRY2 * SPRITE_SIZE)

#define SPRITE_MAX_LOAD (64*KB / SPRITE_CACHE_ENTRY_SIZE)
//#define SPRITES_PER_FRAME (SPRITE_MAX_LOAD*4)

#define TILES_PER_ENTRY 4
#define TILE_SIZE 64
#define TILE_CACHE_ENTRY_SIZE (TILES_PER_ENTRY * TILE_SIZE)
#define TILE_MAX_LOAD (4*KB / TILE_CACHE_ENTRY_SIZE)
//#define TILES_PER_FRAME (TILE_MAX_LOAD*8)

#define CACHE_ENTRY_NULL 0xff00
//#define CACHE_ENTRY_LOADING 0xfffe

/*#define SCREEN_SIZE_X 256
#define SCREEN_SIZE_Y 192

#define SCREEN_OFF_X ((320 - SCREEN_SIZE_X) / 2)
#define SCREEN_OFF_Y ((224 - SCREEN_SIZE_Y) / 2 + 16)

#define SCREEN_MIN_X SCREEN_OFF_X
#define SCREEN_MAX_X (SCREEN_SIZE_X + SCREEN_OFF_X)
#define SCREEN_MIN_Y SCREEN_OFF_Y
#define SCREEN_MAX_Y (SCREEN_SIZE_Y + SCREEN_OFF_Y)*/

extern TNeoVideoBounds g_videoBounds DTCM_BSS;

//#define SCREEN_FXMIN_X g_videoBounds.minX
//#define SCREEN_FXMAX_X g_videoBounds.maxX
//#define SCREEN_FXMIN_Y g_videoBounds.minY
//#define SCREEN_FXMAX_Y g_videoBounds.maxY

extern u16 g_frameCount DTCM_BSS;

#define VCOUNT (*(volatile u16*)0x4000006)

#endif

