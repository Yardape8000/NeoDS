#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoProfiler.h"
#include "NeoCpu.h"
#include "NeoVideo.h"
#include "NeoMemory.h"
#include "NeoIPC.h"
#include "NeoIO.h"
#include "pd4990a.h"

/*u16 neoReadVideo16(u32 a)
{
	switch(a & 0x0e) {
	case 0x00: //vram offset
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x02: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x04: //vram mod
		return g_neo->vramMod;
	case 0x06: //control reg
		return ((248 + g_neo->scanline) << 7) | g_neo->autoAnimationCounter;
	case 0x08: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x0A: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x0C: //vram mod
		return g_neo->vramMod;
	}
	return cpuUnmapped16();
}*/

/*u16 neoReadCtrl116(u32 a)
{
	u16 data;
	switch(a) {
	case 0x300000: //IN0 - ctrl1
	case 0x300001:
		//high byte is joystick
		//low byte is dips
		//bit0F - button 4
		//bit0E - button 3
		//bit0D - button 2
		//bit0C - button 1
		//bit0B - button right
		//bit0A - button left
		//bit09 - button down
		//bit08 - button up
		//bit07
		//bit06
		//bit05
		//bit04
		//bit03
		//bit02
		//bit01
		//bit00 - test switch
		data = 0x00ff | ((u16)g_neo->p1Input << 8);
		return data;
	case 0x300080: //IN4
	case 0x300081:
		//high byte is ?
		//low byte is test stuff
		return 0xff80;
	default:
		return 0x0000;
	}
}*/

/*u16 neoReadCtrl216(u32 a) //ctrl2
{
	//high byte is joystick
	//low byte is nothing
	return 0xffff;
}*/

/*u16 neoReadCtrl316(u32 a) //ctrl3
{
	//high byte is start buttons and memcard status
	//low byte is nothing
	u16 data = 0x8A00 | 0x7000 | (g_neo->startInput << 8); //0x7000 is memcard not present
	//if(g_neo->keys & KEY_START) data &= ~(1 << 8);
	return data; 
}*/

/*u16 neoReadCoin16(u32 a) //coin
{
	//high byte is audio result
	//low byte is other stuff
	const u16 testbit = read_4990_testbit();
	const u16 databit = read_4990_databit();
	u16 data = 0x0104 | (testbit << 6) | (databit << 7) | g_neo->coinInput;
	//if(g_neo->keys & KEY_SELECT) data &= ~0x01;
	return data;
}*/

static void neoForceZ80Nmi()
{
	profilerPush(NEOPROFILE_AUDIOWAIT);
	neoIPCSendCommand(NEOARM7_NMI);
	profilerPop();
}

void neoWriteAudioCommand8(u32 a, u8 d)
{
	if(NEOIPC->globalAudioEnabled) {
		if(!(a & 0x01)) {
			NEOIPC->audioCommand = d;
			NEOIPC->audioCommandPending = 1;
			//systemWriteLine("Audio command8: %02X", d);
			neoForceZ80Nmi();
		} else {
			systemWriteLine("Ignored audio command8: %02X", d);
		}
	}
}

void neoWriteAudioCommand16(u32 a, u16 d)
{
	if(NEOIPC->globalAudioEnabled) {
		if(!(a & 0x01)) {
			NEOIPC->audioCommand = (d >> 8);
			NEOIPC->audioCommandPending = 1;
			//systemWriteLine("Audio command16: %04X", d);
			neoForceZ80Nmi();
		} else {
			systemWriteLine("Ignored audio command16: %04X", d);
		}
	}
}

void neoWriteSystemLatch16(u32 a, u16 d)
{
	const u8 enable = (u8)((a >> 4) & 0x01);
		
	switch((a >> 1) & 0x07) {
	case 0x00:
		g_neo->screenDarkLatch = enable;
		break;
	case 0x01:
		if(g_neo->irqVectorLatch != enable) {
			g_neo->irqVectorLatch = enable;
			if(!enable) {
				neoMemoryLoadBiosVector();
			} else {
				neoMemoryLoadProgramVector();
			}
		}
		break;
	case 0x05:
		if(g_neo->fixedRomLatch != enable) {
			g_neo->fixedRomLatch = enable;
			if(enable) g_neo->fixedBank = 4096;
			else g_neo->fixedBank = 0;
		}
		break;
	case 0x06:
		g_neo->sramProtectLatch = enable;
		break;
	case 0x07:
		neoVideoPaletteBank(enable);
		break;
	default:
		//systemWriteLine("unknown latch: %08X", a);
		break;
	}
}

/*void neoWriteVideo16(u32 a, u16 d)
{
	switch(a & 0x0e) {
	case 0x00: //vram offset
		if(d < 0x8000) {
			g_neo->pVramBase = g_neo->pVram;
			g_neo->vramBaseMask = 0x7fff;
			g_neo->vramOffset = d;
		} else {
			g_neo->pVramBase = g_neo->pSpriteRam;
			g_neo->vramBaseMask = 0x7ff;
			g_neo->vramOffset = d & 0x7ff;
		}
		break;
	case 0x02: //vram data
		g_neo->pVramBase[g_neo->vramOffset] = d;
		g_neo->vramOffset =
			((g_neo->vramOffset + g_neo->vramMod) & g_neo->vramBaseMask);
		break;
	case 0x04: //vram mod
		g_neo->vramMod = d;
		break;
	case 0x06: //control reg
		g_neo->displayControl = d;
		if(d & (1 << 5)) {
			g_neo->displayCounter = g_neo->displayCounterLoad;
		}
		break;
	case 0x08: //irq2 pos1
		g_neo->displayCounterLoad &= 0x0000ffff;
		g_neo->displayCounterLoad |= (u32)d << 16;
		break;
	case 0x0A: //irq2 pos2
		g_neo->displayCounterLoad &= 0xffff0000;
		g_neo->displayCounterLoad |= (u32)d;
		break;
	case 0x0C: //irq ak
		neoSystemIrqAk(d);
		break;
	}
}*/

//void neoWriteWatchdog16(u32 a, u16 d)
//{
//	g_neo->watchdogCounter = 0;
//}
