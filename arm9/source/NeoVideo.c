#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoVideo.h"
#include "NeoMemory.h"
#include "NeoCpu.h"
#include "NeoProfiler.h"
#include "guiBase.h"
#include "LinearHeap.h"

//from NeoVideoFixed.c
void neoFixedInit();
void neoFixedExit();
void ITCM_CODE neoDrawFixed();
void neoLoadTiles();

//from NeoVideoSprite.c
void neoSpriteInit();
void neoSpriteExit();
void neoDrawSprites();
void neoLoadSprites();

//from NeoVideoPalette.c
void neoPaletteInit();
void neoPaletteExit();
void neoUpdatePalette();
void neoLoadPalette();

volatile u32 g_currentFps;
volatile u32 g_frames;

//static u32 g_paletteDirty = 0;
//static bool g_fixedPaletteDirty = false;
u16 g_frameCount DTCM_BSS;
static volatile u16 g_framePending = 0;

static const TNeoVideoBounds g_videoBoundsList[] = {
	{32, (16 + 16) << PIXEL_Y_SHIFT, (256 + 32 - 1), (192 + 16 + 16 - 1) << PIXEL_Y_SHIFT},
	{8, (0 + 16) << PIXEL_Y_SHIFT, (304 + 8 - 1), (224 + 16 - 1) << PIXEL_Y_SHIFT},
};
TNeoVideoBounds g_videoBounds DTCM_BSS;
static TNeoVideoSize g_size = NEOVIDEO_NORMAL;

//static s16 g_zValue = 0;

void neoVideoSetSize(TNeoVideoSize size)
{
	ASSERT(size < NEOVIDEO_SIZECOUNT);
	g_size = size;
}

TNeoVideoSize neoVideoGetSize()
{
	return g_size;
}

/*void neoVideoWritePal8(u32 a, u8 d)
{
	u16 word = g_neo->pPalette[((a >> 1) & 0x0fff) + g_neo->paletteBank];
	if(a & 1) word = (word & 0xff00) | (u16)d;
	else word = (word & 0x00ff) | ((u16)d << 8);

	neoVideoWritePal16(a, word);
}*/

/*void neoVideoWritePal16(u32 a, u16 d)
{
	//const u16 r = ((d >> 7) & 0x1e) | ((d >> 14) & 0x01);
	//const u16 g = ((d >> 3) & 0x1e) | ((d >> 13) & 0x01);
	//const u16 b = ((d << 1) & 0x1e) | ((d >> 12) & 0x01);
	const u16 entry =
		((d >> 7) & 0x1e) | ((d >> 14) & 0x01) |
		((d << 2) & (0x1e << 5)) | ((d >> 8) & (0x01 << 5)) |
		((d << 11) & (0x1e << 10)) | ((d >> 2) & (0x01 << 10));

	const u32 address = (a & 0x1fff);
	const u32 offset = (address >> 1) + g_neo->paletteBank;
	g_neo->pNitroPalette[offset] = entry;//(b << 10) | (g << 5) | r;
	g_neo->pPalette[offset] = d;
	g_neo->paletteDirty |= 1 << (offset >> 8);
}*/

//u16 neoVideoReadPal16(u32 a)
//{
//	return g_neo->pPalette[((a >> 1) & 0x0fff) + g_neo->paletteBank];
//}

static void vblankIntr()
{
	//thanks to kvance from gbadev.org for scaling code
	/*static const u16 jitter2[] = {
		0x40, 0xc0,		// 0.25, 0.75
		0xc0, 0x40,		// 0.75, 0.25
	};*/
	static const u16 jitter4[] = {
		0x60, 0x40,		// 0.375, 0.250 
		0x20, 0xc0,		// 0.125, 0.750
		0xe0, 0x40,		// 0.875, 0.250
		0xa0, 0xc0,		// 0.625, 0.750
	};
	/*static const u16 jitterF4[] = {
		0x60, 0x000,	// 0.375, 0.000
		0x20, 0x100,	// 0.125, 1.000
		0xe0, 0x000,	// 0.875, 0.250
		0xa0, 0x100,	// 0.625, 0.750
	};*/

	void* saveR7 = g_neo;
	neoResetContext(); //reload global register variable

	if(g_framePending == 2) {
		neoLoadSprites();
		neoLoadPalette();
		neoLoadTiles();
		
		g_framePending = 0;
		g_frames++;
	}

	//antialias tile layer
	static u16 sIndex = 0;
	static u16 sTime = 0;
	//offset
	if(g_size != NEOVIDEO_NORMAL) {
		BACKGROUND.bg2_rotation.dx = g_neo->tileOffsetX + jitter4[sIndex + 0];
		BACKGROUND.bg2_rotation.dy = g_neo->tileOffsetY + jitter4[sIndex + 1];
		BACKGROUND.bg3_rotation.dx = g_neo->tileOffsetX + jitter4[sIndex + 2];
		BACKGROUND.bg3_rotation.dy = g_neo->tileOffsetY + jitter4[sIndex + 3];
		sIndex += 4;
		if(sIndex >= 8) sIndex = 0;
	} else {
		BACKGROUND.bg2_rotation.dx = g_neo->tileOffsetX;
		BACKGROUND.bg2_rotation.dy = g_neo->tileOffsetY;
		BACKGROUND.bg3_rotation.dx = g_neo->tileOffsetX;
		BACKGROUND.bg3_rotation.dy = g_neo->tileOffsetY;
	}
	//scale
	BACKGROUND.bg2_rotation.xdx = g_neo->tileScaleX;
	BACKGROUND.bg2_rotation.ydy = g_neo->tileScaleY;
	BACKGROUND.bg3_rotation.xdx = g_neo->tileScaleX;
	BACKGROUND.bg3_rotation.ydy = g_neo->tileScaleY;

	sTime++;
	if(sTime >= 60) {
		sTime = 0;
		g_currentFps = g_frames;
		g_frames = 0;
	}
	g_neo = (TNeoContext*)saveR7;
}

static void vcountIntr()
{
	void* saveR7 = g_neo;
	neoResetContext(); //reload global register variable

	if(g_framePending == 1 && VCOUNT < 192) {
		GFX_FLUSH = 0;
		g_framePending = 2;
	}
	g_neo = (TNeoContext*)saveR7;
}

bool neoVideoInit()
{
	g_neo->pVramBase = g_neo->pVram;
	g_neo->vramBaseMask = 0x7fff;

	vramSetBankE(VRAM_E_MAIN_BG); //fixed bg and tiles
	vramSetBankF(VRAM_F_LCD);
	vramSetBankG(VRAM_G_LCD); //texture palette
	vramSetBankH(VRAM_H_LCD);
	
	//set mode for main screen (3d sprites + char bg for fixed layer)
	videoSetMode(
		MODE_5_3D |
		DISPLAY_BG2_ACTIVE |
		DISPLAY_BG3_ACTIVE |
		DISPLAY_BG_EXT_PALETTE
	);
	
	//set 3d priority to be below fixed layer priority
	REG_BG0CNT = BG_PRIORITY_2;

	GFX_CONTROL = 1; //turn on texture mapping
	GFX_VIEWPORT =
		0 << 0 | //x0
		0 << 8 | //y0
		255 << 16 | //x1
		191 << 24; //y1

	GFX_POLY_FORMAT =
		(1 << 6) | //render back
		(1 << 7) | //render front
		(31 << 16); //alpha

	GFX_COLOR = 0x7fff; //white

	GFX_CLEAR_COLOR = 0; //disabled
	GFX_CLEAR_DEPTH = 0x7fff;

	//reset matrix stack
	MATRIX_CONTROL = 0;
	MATRIX_IDENTITY = 0;
	MATRIX_CONTROL = 1;
	MATRIX_IDENTITY = 0;
	MATRIX_CONTROL = 2;
	MATRIX_IDENTITY = 0;
	MATRIX_CONTROL = 3;
	MATRIX_IDENTITY = 0;

	neoFixedInit();
	neoSpriteInit();
	neoPaletteInit();

	SetYtrigger(190); //trigger 2 lines before vsync
	irqSet(IRQ_VBLANK, vblankIntr);
	irqSet(IRQ_VCOUNT, vcountIntr);

	g_frameCount++;

	systemWriteLine("Video initialized");
	g_videoBounds = g_videoBoundsList[g_size];

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof32(
		g_videoBounds.minX, g_videoBounds.maxX,
		g_videoBounds.maxY, g_videoBounds.minY,
		0, 1 << 12);
	glMatrixMode(GL_MODELVIEW);

	return true;
}

void neoVideoExit()
{
	neoFixedExit();
	neoSpriteExit();
	neoPaletteExit();
}

//bool neoVideoIsFramePending()
//{
//	return g_framePending;
//}

/*void neoVideoFinishFrame()
{
	GFX_FLUSH = 0;
	swiWaitForVBlank();
	
	//REG_EXMEMCNT &= ~ARM7_MAIN_RAM_PRIORITY;
	neoLoadSprites();
	neoLoadPalette();
	neoLoadTiles();
	//REG_EXMEMCNT |= ARM7_MAIN_RAM_PRIORITY;
	
	g_framePending = false;
	g_frames++;
}*/

extern void guiConsoleDump();

void neoVideoDrawFrame()
{
	const TNeoVideoBounds* pTarget = &g_videoBoundsList[g_size];
	//static u32 lastFrameTime = 0;
	//static u32 skipCount = 0;
	//u32 frameTime;
	//u32 deltaTime;

	g_frameCount++;

	while(g_framePending > 0) {
		//never got around to finishing last frame...do it now
		//neoVideoFinishFrame();
		//swiWaitForVBlank();
		swiIntrWait(0, IRQ_VBLANK);
	}

	if(g_videoBounds.minX != pTarget->minX || g_videoBounds.maxX != pTarget->maxX ||
		g_videoBounds.minY != pTarget->minY || g_videoBounds.maxY != pTarget->maxY) {
			if(g_videoBounds.minX < pTarget->minX) g_videoBounds.minX++;
			else if(g_videoBounds.minX > pTarget->minX) g_videoBounds.minX--;

			if(g_videoBounds.maxX < pTarget->maxX) g_videoBounds.maxX++;
			else if(g_videoBounds.maxX > pTarget->minX) g_videoBounds.maxX--;

			if(g_videoBounds.minY < pTarget->minY) g_videoBounds.minY += (1 << PIXEL_Y_SHIFT);
			else if(g_videoBounds.minY > pTarget->minY) g_videoBounds.minY -= (1 << PIXEL_Y_SHIFT);

			if(g_videoBounds.maxY < pTarget->maxY) g_videoBounds.maxY += (1 << PIXEL_Y_SHIFT);
			else if(g_videoBounds.maxY > pTarget->minY) g_videoBounds.maxY -= (1 << PIXEL_Y_SHIFT);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrthof32(
				g_videoBounds.minX, g_videoBounds.maxX,
				g_videoBounds.maxY, g_videoBounds.minY,
				0, 1 << 12);
			glMatrixMode(GL_MODELVIEW);
	}

	//DC_FlushRange(&g_neo->pNitroPalette[g_neo->paletteBank], 32*256);

	guiSystemRender();
	
	neoUpdatePalette();

	if(g_neo->sramProtectCount > 0) {
		profilerPush(NEOPROFILE_VIDEO_SPRITES);
		neoDrawSprites();
		profilerPop();
	}

	profilerPush(NEOPROFILE_VIDEO_FIXED);
	neoDrawFixed();
	profilerPop();

	g_framePending = 1;
}
