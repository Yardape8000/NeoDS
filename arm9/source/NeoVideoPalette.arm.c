#include "Default.h"
#include "NeoVideo.h"
#include "NeoMemory.h"

static u16* g_nitroPalette;
static u32 g_startTransfer;
static u32 g_endTransfer;
static u32 g_loadFixed;
static u32 g_paletteBank = 0xffffffff;

void neoVideoPaletteBank(u32 enable)
{
	if(g_neo->paletteRamLatch != enable) {
		g_neo->paletteRamLatch = enable;
		if(enable) g_neo->paletteBank = 4*KB;
		else g_neo->paletteBank = 0;
		//g_neo->fixedPaletteDirty = true;
		//g_paletteDirty = 0xffff;
	}
}

void neoPaletteInit()
{
	g_nitroPalette = (u16*)neoSystemVramHAlloc(16*KB);
}

void neoPaletteExit()
{

}

void neoUpdatePalette()
{
	const u16* restrict pSrc = &g_neo->pPalette[g_neo->paletteBank];
	u16* restrict pDst = &g_nitroPalette[g_neo->paletteBank];
	const u32 start = (g_neo->paletteBank == 0) ? 0 : 8;
	u32 i;
	u32 j;
	u32 p;

	g_startTransfer = 0xffffffff;
	g_endTransfer = 0;
	//if any of the first 16 palettes are dirty
	g_loadFixed = *((u16*)&g_neo->paletteDirty[start]);
	if(g_neo->paletteBank != g_paletteBank) {
		g_loadFixed = 1;
	}
	g_paletteBank = g_neo->paletteBank;
	
	for(i = start; i < start + 8; i++) {
		const u32 dirty = g_neo->paletteDirty[i];
		if(dirty == 0) {
			pSrc += 512;
			pDst += 512;
			continue;
		}
		g_neo->paletteDirty[i] = 0;
		for(j = 0; j < 32; j++) {
			const u32 index = i * 32 + j;
			if(index < g_startTransfer) g_startTransfer = index;
			//just always set this, last one will "stick"
			g_endTransfer = index;
			if(dirty & (1 << j)) {
				for(p = 0; p < 16; p++) {
					const u32 d = (u32)*pSrc++;
					*pDst++ =
						((d >> 7) & 0x1e) | ((d >> 14) & 0x01) |
						((d << 2) & (0x1e << 5)) | ((d >> 8) & (0x01 << 5)) |
						((d << 11) & (0x1e << 10)) | ((d >> 2) & (0x01 << 10));
				}
			} else {
				pSrc += 16;
				pDst += 16;
			}
		}
	}

	GFX_CLEAR_COLOR = g_nitroPalette[g_neo->paletteBank + 0x0fff] | (0x1f << 16);
}

void neoLoadPalette()
{
	if(g_endTransfer > g_startTransfer) {
		const u16* restrict pSrc = &g_nitroPalette[g_startTransfer * 16];
		u16* restrict pDst = VRAM_G + g_startTransfer * 16;
		const u32 transferSize = (g_endTransfer - g_startTransfer + 1) * 32;
		vramSetBankG(VRAM_G_LCD);
		dmaCopyWords(3, pSrc, pDst, transferSize);
		vramSetBankG(VRAM_G_TEX_PALETTE);
	}

	if(g_loadFixed) {
		const u32* restrict pSrc = (const u32*)&g_nitroPalette[g_paletteBank];
		u32* restrict pDst0 = (u32*)(VRAM_F); //write to slot2
		u32* restrict pDst1 = (u32*)((u8*)VRAM_F + 8*KB); //write to slot3
		s32 i;

		vramSetBankF(VRAM_F_LCD);
		for(i = 0; i < 16; i++) {
			//copy 16 colors
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			*pDst0++ = *pSrc++;
			pSrc -= 8;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			*pDst1++ = *pSrc++;
			//advance to next 256 color palette
			pDst0 += 128 - 8;
			pDst1 += 128 - 8;
		}
		//dmaCopyWords(3, &g_nitroPalette[g_neo->paletteBank], &BG_PALETTE[0], 512);
		vramSetBankF(VRAM_F_BG_EXT_PALETTE | VRAM_OFFSET(1)); //assign to slots 2, 3
		DC_InvalidateRange(&g_nitroPalette[g_paletteBank], 512);
	}
}
