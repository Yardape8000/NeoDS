#include "Default.h"
#include "guiRender.h"

//from data/gui.bin
extern const u8 gui_bin[];
extern const int gui_bin_size;

#define GUI_BUFFER_SIZE (GUI_WIDTH * GUI_HEIGHT)
#define BACKBUFFER_VRAM ((u16*)SCREEN_BASE_BLOCK_SUB(21)) //42k
#define TEXTBUFFER_VRAM ((u16*)SCREEN_BASE_BLOCK_SUB(20)) //40k
#define TILEGFX_VRAM ((u16*)CHAR_BASE_BLOCK_SUB(2)) //32k

#define RGB888(r, g, b) RGB15((r)>>3, (g)>>3, (b)>>3)
#define GRAY8(x) RGB888(x, x, x)

static const u16 g_palette[256] = {
	RGB888(0, 0, 0),
	GRAY8(152),
	GRAY8(32),
	GRAY8(120),
	GRAY8(96),
	GRAY8(72),
	GRAY8(176),
	GRAY8(200),
	GRAY8(224),
	GRAY8(112),
	GRAY8(248),
	GRAY8(0),
	GRAY8(0),
	GRAY8(0),
	GRAY8(0),
	GRAY8(0),

	RGB888(255, 0, 255),
	GRAY8(0),
	GRAY8(255),
	GRAY8(128),
	GRAY8(96),
};

void guiRenderInit()
{
	s32 i;

	vramSetBankI(VRAM_I_SUB_BG); //text screen
	//set mode for subscreen (text)
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);

	//text screen
	SUB_BG0_CR = BG_MAP_BASE(20) | BG_TILE_BASE(2);
	SUB_BG0_X0 = -4;
	SUB_BG0_Y0 = -4;

	//background screen
	SUB_BG1_CR = BG_MAP_BASE(21) | BG_TILE_BASE(2);
	SUB_BG1_X0 = 0;
	SUB_BG1_Y0 = 0;

	//palette
	for(i = 0; i < 256; i++) {
		BG_PALETTE_SUB[i] = g_palette[i];
	}

	DC_FlushAll();

	//load vram
	dmaCopy(gui_bin, TILEGFX_VRAM, gui_bin_size);
}

void guiRenderFrameBounds(const TBounds* pBounds, TGuiBorderStyle style)
{
	const s32 w = pBounds->x1 - pBounds->x0 + 1;
	const s32 h = pBounds->y1 - pBounds->y0 + 1;

	guiRenderFrame(pBounds->x0, pBounds->y0, w, h, style);
}

void guiRenderLogo(s32 x0, s32 y0)
{
	const u32 width = 12;
	const u32 stride = GUI_WIDTH - width;
	u16* pDst = &BACKBUFFER_VRAM[y0 * GUI_WIDTH + x0];
	s32 x;

	u16 tile = 176;
	for(x = 0; x < width; x++) {
		*pDst++ = tile++;
	}
	pDst += stride;
	tile = 192;
	for(x = 0; x < width; x++) {
		*pDst++ = tile++;
	}
	pDst += stride;
	tile = 208;
	for(x = 0; x < width; x++) {
		*pDst++ = tile++;
	}
}

void guiRenderFrame(s32 x, s32 y, s32 w, s32 h, TGuiBorderStyle style)
{
	s32 x0;
	s32 y0;
	u16* pDst = &BACKBUFFER_VRAM[y * GUI_WIDTH + x];
	u32 tile = 0;

	switch(style) {
	case GUIBORDER_NORMAL: tile = 131; break;
	case GUIBORDER_PRESSED: tile = 128; break;
	case GUIBORDER_ROUND_NORMAL: tile = 137; break;
	case GUIBORDER_ROUND_PRESSED: tile = 134; break;
	}

	*pDst++ = tile;
	for(x0 = 1; x0 < w - 1; x0++) {
		*pDst++ = tile + 1;
	}
	*pDst++ = tile + 2;
	pDst += GUI_WIDTH - w;

	for(y0 = 1; y0 < h - 1; y0++) {
		*pDst++ = tile + 16;
		for(x0 = 1; x0 < w - 1; x0++) {
			*pDst++ = tile + 17;
		}
		*pDst++ = tile + 18;
		pDst += GUI_WIDTH - w;
	}

	*pDst++ = tile + 32;
	for(x0 = 1; x0 < w - 1; x0++) {
		*pDst++ = tile + 33;
	}
	*pDst++ = tile + 34;
}

void guiRenderChar(s32 x, s32 y, char c)
{
	ASSERT(x >= 0 && y >= 0);
	ASSERT(x < GUI_WIDTH && y < GUI_HEIGHT);
	u16* pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH + x];
	*pDst = c;
}

void guiRenderVertString(s32 x, s32 y, const char* szString)
{
	ASSERT(x >= 0 && y >= 0);
	ASSERT(x < GUI_WIDTH && y < GUI_HEIGHT);
	u16* pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH + x];
	
	while(*szString) {
		*pDst = *szString++;
		pDst += GUI_WIDTH;
		y++;
		if(x >= GUI_HEIGHT - 1) {
			y = 0;
			x++;
			pDst = &TEXTBUFFER_VRAM[x];
		}
	}
}


void guiRenderString(s32 x, s32 y, const char* szString)
{
	ASSERT(x >= 0 && y >= 0);
	ASSERT(x < GUI_WIDTH && y < GUI_HEIGHT);
	u16* pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH + x];
	
	while(*szString) {
		*pDst++ = *szString++;
		x++;
		if(x >= GUI_WIDTH - 1) {
			x = 0;
			y++;
			pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH];
		}
	}
}

void guiRenderStringn(s32 x, s32 y, u32 n, const char* szString)
{
	ASSERT(x >= 0 && y >= 0);
	ASSERT(x < GUI_WIDTH && y < GUI_HEIGHT);
	u16* pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH + x];
	
	while(*szString && n > 0) {
		*pDst++ = *szString++;
		x++;
		n--;
		if(x >= GUI_WIDTH - 1) {
			x = 0;
			y++;
			pDst = &TEXTBUFFER_VRAM[y * GUI_WIDTH];
		}
	}
}

void guiRenderClearBounds(const TBounds* pBounds)
{
	s32 width = pBounds->x1 - pBounds->x0 + 1;
	s32 x, y;

	for(y = pBounds->y0; y <= pBounds->y1; y++) {
		u16* pDst0 = &BACKBUFFER_VRAM[y * GUI_WIDTH + pBounds->x0];
		u16* pDst1 = &TEXTBUFFER_VRAM[y * GUI_WIDTH + pBounds->x0];
		for(x = 0; x < width; x++) {
			*pDst0++ = 0;
			*pDst1++ = 0;
		}
	}
}


void guiRenderClear()
{
	u16* pDst0 = &BACKBUFFER_VRAM[0];
	u16* pDst1 = &TEXTBUFFER_VRAM[0];
	s32 i;
	for(i = 0; i < GUI_BUFFER_SIZE; i++) {
		*pDst0++ = 0;
		*pDst1++ = 0;
	}
}
