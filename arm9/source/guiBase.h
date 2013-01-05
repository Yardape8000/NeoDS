#ifndef _GUI_BASE_H
#define _GUI_BASE_H

#include "guiObject.h"
#include "guiRender.h"

#define GUI_MAX_TEXT 32

void guiSystemInit();
void guiSystemRender();
void guiSystemProcess();
void guiFramePush_r(const TGuiTypeHeader* pType);
void guiFrameNew_r(const TGuiTypeHeader* pType);
#define guiFramePush(typeof) guiFramePush_r(&GUITYPE_HEADER(typeof))
#define guiFrameNew(typeof) guiFrameNew_r(&GUITYPE_HEADER(typeof))
void guiFramePop();
TGuiObject* guiGetRoot();
void* guiHeapAlloc(u32 size);
void guiBroadcastEvent(TGuiEventID e, void* arg);

TGuiObject* guiObjAllocChild_r(const TGuiTypeHeader* pType, TGuiObject* pParent,
							   const TBounds* pBounds);
TGuiObject* guiObjCreateChild_r(const TGuiTypeHeader* pType, TGuiObject* pParent,
					const TBounds* pBounds);

#define guiObjCreateChild(typeof, pParent, pBounds) \
	((typeof*)guiObjCreateChild_r(&GUITYPE_HEADER(typeof), pParent, pBounds))
#define guiObjCreate(typeof, pBounds) \
	((typeof*)guiObjCreateChild_r(&GUITYPE_HEADER(typeof), guiGetRoot(), pBounds))


#endif
