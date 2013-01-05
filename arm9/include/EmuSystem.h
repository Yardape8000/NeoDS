#ifndef _EMU_SYSTEM_H
#define _EMU_SYSTEM_H

#define VRAM_CODE __attribute__((section(".vram"), long_call))

//typedef s32 fx32;
//typedef s16 fx16;

//#define FX_SHIFT 12

//#define toFx32(i) (i << FX_SHIFT)

//#define FX_ONE (1 << FX_SHIFT)
//#define FX_TWO (2 << FX_SHIFT)

#define UNCACHED(a) ((void*)((u32)(a)|0x400000))
#define OFFSET(typeof, field) ((u32)(&((typeof*)0)->field) - (u32)((typeof*)0))

//more cache functions, defined in ExtCache.s
//void DC_StoreRange(register void *base, register u32 size);
//void DC_LockdownRange(register const void* startAddr, register u32 nBytes);
//void DC_UnlockdownAll();

bool systemInit();
bool systemSelectRom(char* szName);

#ifndef NEO_SHIPPING
void systemWriteLine(const char* szLine, ...);
#else
#define systemWriteLine(...) ((void)0)
#endif

void systemPanic_d(const char* szFile, u32 line, const char* szMessage, ...);
u32 systemGetMs();
void systemWaitKey(const char* szText);

//void systemRamLock();
//volatile void* systemRamUnlock();
//u32 systemGetRamSize();

void* systemAlloc(u32 size);
void* systemRealloc(void* p, u32 size);
void systemFree(void* p);

void* systemSlot2Alloc(u32 size);
void systemSlot2Reset();
u32 systemGetSlot2Free();
void systemSlot2Unlock();
void systemSlot2Lock();
bool systemIsSlot2Active();

void* systemRamAlloc(u32 size);
u32 systemGetRamFree();
void systemRamReset();

s32 systemOpen(const char* szFileName, bool write);
void systemRead(s32 file, void* pDst, u32 size);
void systemReadOffset(s32 file, void* pDst, u32 offset, u32 size);
void systemWrite(s32 file, const void* pSrc, u32 size);
void systemClose(s32 file);
void systemSeek(s32 file, s32 offset, bool relative);
u32 systemFileSize(s32 file);

#define systemPanic(msg, ...) systemPanic_d(__FILE__, __LINE__, msg, __VA_ARGS__)

#ifndef NEO_SHIPPING
#	define ASSERT(x) (void)((x) || (systemPanic("Assert: %s", #x), 0))
#	define ASSERTMSG(x, ...) (void)((x) || \
		(systemWriteLine("Assert: %s", #x), systemPanic_d(__FILE__, __LINE__, __VA_ARGS__), 0))
#else
#	define ASSERT(x) ((void)0)
#	define ASSERTMSG(x, ...) ((void)0)
#endif

#define KB (1<<10)
#define MB (1<<20)

extern volatile u32 g_currentFps;
extern volatile u32 g_frames;

#endif
