#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoCpu.h"
#include "NeoVideo.h"
#include "NeoIPC.h"
#include "NeoAudioStream.h"
#include "guiBase.h"
#include "LayoutRomSelect.h"
#include "LayoutMain.h"

STATIC_ASSERT(sizeof(TransferRegion) < 0x800);

/*extern u16 __vram_lma[];
extern u16 __vram_start[];
extern u16 __vram_end[];

extern int VRAM_CODE vramCodeTest();

void neoCrt0()
{
	const u16* pSrc = __vram_lma;
	const u32 vramSize = (u32)__vram_end - (u32)__vram_start;
	u16* pDst = __vram_start;
	u32 i;

	powerON(POWER_ALL);
	vramSetBankE(VRAM_E_LCD);
	
	systemWriteLine("vramStart: %08X", (u32)__vram_start);
	systemWriteLine("vramLma: %08X", (u32)__vram_lma);
	systemWriteLine("vramEnd: %08X", (u32)__vram_end);
	systemWriteLine("vram size: %d", vramSize);

	for(i = 0; i < vramSize / 2; i++) {
		*pDst++ = *pSrc++;
	}

	systemWriteLine("Result: %d", vramCodeTest());

	for(i = 0; i < 60; i++) {
		swiWaitForVBlank();
	}
}*/

//extern u16 Z80DaaTable[];

int main(void)
{
	neoClearContext();
	DC_FlushAll();
	
	memset((void*)NEOIPC, 0, sizeof(TNeoIPC));
	//NEOIPC->arm7Ready = 0;
	//NEOIPC->arm9Ready = 0;
	NEOIPC->audioCommand = 0;
	NEOIPC->audioResult = 0;
	//NEOIPC->audioEnabled = 1;
	NEOIPC->globalAudioEnabled = 1;
	//NEOIPC->pZ80DaaTable = Z80DaaTable;

	neoIPCInit();

	guiSystemInit();

	bool initOk = systemInit();
	if(!initOk) {
		systemPanic("systemInit failed", 0);
		return 0;
	}

	//neoCrt0();

	neoSystemInit();

	guiFrameNew(TGuiLayoutMain);
	guiSystemProcess();
	guiFramePush(TGuiLayoutRomSelect);

	g_neo->active = false;

	neoSystemExecute();

	return 0;
}
