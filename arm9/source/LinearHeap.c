#include "Default.h"
#include "LinearHeap.h"

void linearHeapInit(TLinearHeap* pHeap, void* pData, u32 size)
{
	ASSERT(pHeap);
	pHeap->pBase = (u8*)pData;
	pHeap->pAlloc = (u8*)pData;
	pHeap->pTop = pHeap->pBase + size;
}

void* linearHeapAlloc(TLinearHeap* pHeap, u32 size)
{
	ASSERT(pHeap);
	ASSERT(size > 0);
	if(pHeap->pAlloc + size > pHeap->pTop) {
		ASSERTMSG(0, "linearHeapAlloc: heap overflow (%d / %d)",
			size, linearHeapGetFree(pHeap));
		return NULL;
	}
	void* pData = pHeap->pAlloc;
	pHeap->pAlloc += size;
	return pData;
}

void linearHeapReset(TLinearHeap* pHeap)
{
	ASSERT(pHeap);
	pHeap->pAlloc = pHeap->pBase;
}

void linearHeapClear(const TLinearHeap* pHeap)
{
	ASSERT(pHeap);
	u16* pDst = (u16*)pHeap->pBase;
	while((u32)pDst < (u32)pHeap->pTop) {
		*pDst++ = 0;
	}
}

u32 linearHeapGetFree(const TLinearHeap* pHeap)
{
	ASSERT(pHeap);
	return (u32)pHeap->pTop - (u32)pHeap->pAlloc;
}

