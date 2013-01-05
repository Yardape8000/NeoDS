#include "Default.h"
#include <fat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include "ram.h"

#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoVideo.h"
#include "NeoIPC.h"
#include "NeoAudioStream.h"
#include "LinearHeap.h"
#include "guiConsole.h"

static volatile u32 g_ms;
static TLinearHeap g_slot2Heap;
static TLinearHeap g_ramHeap;
static u8 g_mainRam[1*MB+600*KB] ALIGN(32);

/*int VRAM_CODE vramCodeTest()
{
	volatile int i;
	int j = 0;

	for(i = 0; i < 300; i++) {
		j = (j * i) + 1 + i;
	}
	return j;
}*/

static void timer0Intr()
{
	g_ms++;
}

u32 systemGetMs()
{
	return g_ms;
}

void* systemAlloc(u32 size)
{
	void* pRet = malloc(size);
	ASSERTMSG(pRet, "systemAlloc - failed");
	return pRet;
}

void* systemRealloc(void* p, u32 size)
{
	void* pRet = realloc(p, size);
	ASSERTMSG(pRet, "systemRealloc - failed");
	return pRet;
}

s32 systemOpen(const char* szFileName, bool write)
{
	int mode = write ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY;
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	int fd = open(szFileName, mode);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
	return fd;
}

void systemReadOffset(s32 file, void* pDst, u32 offset, u32 size)
{
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	lseek(file, offset, SEEK_SET);
	read(file, pDst, size);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
}

void systemRead(s32 file, void* pDst, u32 size)
{
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	read(file, pDst, size);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
}

void systemSeek(s32 file, s32 offset, bool relative)
{
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	lseek(file, offset, relative ? SEEK_CUR : SEEK_SET);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
}

void systemWrite(s32 file, const void* pSrc, u32 size)
{
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	while(size > 0) {
		u32 sizeToWrite = size;
		if(sizeToWrite > 512) sizeToWrite = 512;
		write(file, pSrc, sizeToWrite);
		size -= sizeToWrite;
		pSrc += sizeToWrite;
	}
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	//write(file, pSrc, size);
	neoResetContext();
}

void systemClose(s32 file)
{
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	close(file);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
}

u32 systemFileSize(s32 file)
{
	struct stat fileStat;
	irqDisable(IRQ_VBLANK | IRQ_VCOUNT);
	fstat(file, &fileStat);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	neoResetContext();
	return fileStat.st_size; 
}

bool systemInit()
{
	irqInit();
	irqSet(IRQ_TIMER0, timer0Intr);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_TIMER0 | IRQ_IPC_SYNC | IRQ_DMA3);

	TIMER_DATA(0) = TIMER_FREQ_1024(1000);
	TIMER_CR(0) = TIMER_DIV_1024 | TIMER_ENABLE | TIMER_IRQ_REQ;

	powerON(POWER_ALL);

	systemWriteLine("fatInit...");
	bool fatOk = fatInit(8, true);
	if(!fatOk) {
		guiConsoleLog("fatInit failed!");
		return false;
	}

	bool ramOk = ram_init(DETECT_RAM);
	if(ramOk) {
		volatile void* pSlot2Ram = ram_unlock();
		u32 slot2Size = ram_size();
		linearHeapInit(&g_slot2Heap, (void*)pSlot2Ram, slot2Size);
		//ram_lock();
		systemWriteLine("Found %d bytes of slot2ram", slot2Size);
		ram_lock();
	} else {
		systemWriteLine("No slot2ram");
		linearHeapInit(&g_slot2Heap, 0, 0);
	}
	linearHeapInit(&g_ramHeap, g_mainRam, sizeof(g_mainRam));

	return true;
}

bool systemIsSlot2Active()
{
	return ram_size() > 0;
}

void* systemRamAlloc(u32 size)
{
	return linearHeapAlloc(&g_ramHeap, size);
}

u32 systemGetRamFree()
{
	return linearHeapGetFree(&g_ramHeap);
}

void systemRamReset()
{
	linearHeapReset(&g_ramHeap);
}

void* systemSlot2Alloc(u32 size)
{
	return linearHeapAlloc(&g_slot2Heap, size);
}

u32 systemGetSlot2Free()
{
	return linearHeapGetFree(&g_slot2Heap);
}

void systemSlot2Reset()
{
	linearHeapReset(&g_slot2Heap);
	systemWriteLine("Slot2 reset: %d bytes", systemGetSlot2Free());
}

void systemSlot2Unlock()
{
	ram_unlock();
}

void systemSlot2Lock()
{
	ram_lock();
}

#ifndef NEO_SHIPPING
void systemWriteLine(const char* szLine, ...)
{
	va_list v;

	va_start(v, szLine);
	guiConsoleLogfv(szLine, v);
	va_end(v);
}
#endif

void systemPanic_d(const char* szFile, u32 line, const char* szMessage, ...)
{
	va_list v;

	guiConsoleLog("*** PANIC ***");
	guiConsoleLogf("%s: %d", szFile, line);
	va_start(v, szMessage);
	guiConsoleLogfv(szMessage, v);
	va_end(v);

	guiConsoleDump();

	REG_IME = 0;
	while(1) continue;
}

void systemWaitKey(const char* szText)
{
	u32 keys = 0;
	
	scanKeys();
	keysDown();
	systemWriteLine("%s: Wait", szText);
	while(keys == 0) {
		swiWaitForVBlank();
		scanKeys();
		keys = keysDown();
		if(keys & KEY_START) {
			g_neo->debug = false;
		}
	}
	systemWriteLine(" -> Pressed");
}

/*void smallVsprintf(char* szBuffer, u32 bufferSize, const char* szFormat, va_list v)
{
	const char* pFormat = szFormat;
	char* pBuffer = szBuffer;
	char* pBufferEnd = pBuffer + bufferSize;
	const char* vs;
	int vd;

	while(1) {
		char c = *pFormat++;
		if(c == 0) {
			break;
		} else if(c == '%') {
			char command = *pFormat++;
			switch(command) {
			case 's':
				vs = va_arg(v, const char*);
				while(1) {
					char c0 = *vs++;
					if(c0 == 0) break;
					*pBuffer++ = c0;
					if(pBuffer >= pBufferEnd) {
						break;
					}
				}
				break;
			case 'd':



			}
		} else {
			*pBuffer++ = c;
			if(pBuffer >= pBufferEnd) {
				break;
			}
		}
	}
}*/

