#ifndef _LINEAR_HEAP_H
#define _LINEAR_HEAP_H

typedef struct _TLinearHeap {
	u8* pBase;
	u8* pAlloc;
	u8* pTop;
} TLinearHeap;

void linearHeapInit(TLinearHeap* pHeap, void* pData, u32 size);
void* linearHeapAlloc(TLinearHeap* pHeap, u32 size);
void linearHeapReset(TLinearHeap* pHeap);
void linearHeapClear(const TLinearHeap* pHeap);
u32 linearHeapGetFree(const TLinearHeap* pHeap);

#endif
