#include "Default.h"
#include "NeoProfiler.h"
#include "guiConsole.h"
#include "guiBase.h"

#define GUI_HEAP_SIZE (16*KB)
#define GUI_STACK_SIZE 4

typedef struct _TGuiFrameEntry {
	TGuiObject* pRoot;
	u32 heapPos;
} TGuiFrameEntry;

typedef enum _TGuiSystemCommand {
	GUICOMMAND_NONE,
	GUICOMMAND_PUSH,
	GUICOMMAND_POP,
	GUICOMMAND_NEW,
} TGuiSystemCommand;


static const TBounds g_guiBounds = {{0, 0, GUI_WIDTH - 1, GUI_HEIGHT - 1}};
static u8 g_heap[GUI_HEAP_SIZE] ALIGN(32);
static u32 g_heapPos = 0;
static TGuiFrameEntry g_stack[GUI_STACK_SIZE];
static u32 g_stackPos = 0;

static const TGuiTypeHeader* g_layoutType = NULL;
static TGuiSystemCommand g_command = GUICOMMAND_NONE;

void guiSystemInit()
{
	s32 i;

	g_stackPos = 0;
	g_heapPos = 0;
	memset(g_stack, 0, sizeof(TGuiFrameEntry) * GUI_STACK_SIZE);
	for(i = 0; i < GUI_STACK_SIZE; i++) {
		g_stack[i].pRoot = NULL;
		g_stack[i].heapPos = 0;
	}
	guiRenderInit();
	guiConsoleLog("guiSystemInit complete");
}

void guiFramePush_r(const TGuiTypeHeader* pType)
{
	ASSERTMSG(g_stackPos + 1 < GUI_STACK_SIZE, "guiStack overflow");
	g_layoutType = pType;
	g_command = GUICOMMAND_PUSH;
}

void guiFrameNew_r(const TGuiTypeHeader* pType)
{
	ASSERTMSG(g_stackPos + 1 < GUI_STACK_SIZE, "guiStack overflow");
	g_layoutType = pType;
	g_command = GUICOMMAND_NEW;
}

void guiFramePop()
{
	ASSERTMSG(g_stackPos > 0, "guiStack underflow");
	g_command = GUICOMMAND_POP;
}

TGuiObject* guiGetRoot()
{
	TGuiFrameEntry* pFrame = &g_stack[g_stackPos];
	return pFrame->pRoot;
}

void* guiHeapAlloc(u32 size)
{
	void* pData = &g_heap[g_heapPos];
	g_heapPos += (size + 3) & ~3;
	ASSERTMSG(g_heapPos < GUI_HEAP_SIZE, "guiHeap overflow");
	return pData;
}

TGuiObject* guiFindObjectAt(s32 xPos, s32 yPos)
{
	TGuiFrameEntry* pFrame = &g_stack[g_stackPos];
	TGuiObject* pParent = NULL;
	TGuiObject* pObj = pFrame->pRoot->pChildren;

	while(pObj) {
		if(boundsTest(&pObj->bounds, xPos, yPos)) {
			pParent = pObj;
			xPos -= pObj->bounds.x0;
			yPos -= pObj->bounds.y0;
			pObj = pObj->pChildren;
		} else {
			pObj = pObj->pNextChild;
		}
	}
	return pParent;
}

static void guiSendEventRecursive(TGuiObject* this, TGuiEventID e, void* arg)
{
	TGuiObject* pChild = this->pChildren;
	guiObjSendEvent(this, e, arg);
	while(pChild) {
		guiSendEventRecursive(pChild, e, arg);
		pChild = pChild->pNextChild;
	}
}

void guiBroadcastEvent(TGuiEventID e, void* arg)
{
	TGuiFrameEntry* pFrame = &g_stack[g_stackPos];
	TGuiObject* this = pFrame->pRoot;
	if(this != NULL) {
		guiSendEventRecursive(this, e, arg);
	}
}

void guiSystemRender()
{
	guiBroadcastEvent(GUIEVENT_RENDER, NULL);
}

static void guiSystemProcessCommand()
{
	TGuiFrameEntry* pFrame;

	switch(g_command) {
	case GUICOMMAND_NONE:
		return;
	case GUICOMMAND_PUSH:
		guiRenderClear();
		guiBroadcastEvent(GUIEVENT_DISABLE, NULL);
		g_stackPos++;
		pFrame = &g_stack[g_stackPos];
		pFrame->heapPos = g_heapPos;
		pFrame->pRoot = guiObjAllocChild_r(g_layoutType, NULL, &g_guiBounds);
		//send creation event (after frame is setup)
		guiObjSendEvent(pFrame->pRoot, GUIEVENT_CREATE, NULL);
		//also send enable event
		guiObjSendEvent(pFrame->pRoot, GUIEVENT_ENABLE, NULL);
		break;
	case GUICOMMAND_POP:
		//first disable
		guiBroadcastEvent(GUIEVENT_DISABLE, NULL);
		//then destroy
		guiBroadcastEvent(GUIEVENT_DESTROY, NULL);
		guiRenderClear();
		pFrame = &g_stack[g_stackPos];
		g_heapPos = pFrame->heapPos;
		g_stackPos--;
		guiBroadcastEvent(GUIEVENT_ENABLE, NULL);
		break;
	case GUICOMMAND_NEW:
		guiBroadcastEvent(GUIEVENT_DISABLE, NULL);
		guiBroadcastEvent(GUIEVENT_DESTROY, NULL);
		guiRenderClear();
		pFrame = &g_stack[g_stackPos];
		g_heapPos = pFrame->heapPos;
		pFrame->pRoot = guiObjAllocChild_r(g_layoutType, NULL, &g_guiBounds);
		//send creation event (after frame is setup)
		guiObjSendEvent(pFrame->pRoot, GUIEVENT_CREATE, NULL);
		//and enable
		guiObjSendEvent(pFrame->pRoot, GUIEVENT_ENABLE, NULL);
		break;
	}
	g_command = GUICOMMAND_NONE;
	g_layoutType = NULL;
}

void guiSystemProcess()
{
	static TGuiObject* pTouchDownObj = NULL;
	static s32 keyTime[16] = {0};
	TGuiEventReturn ret = GUIEVENTRET_NOTHANDLED;
	TGuiObject* pTouchObj;
	TGuiObject* pObj;
	touchPosition touch;
	u32 keyRepeat;
	u32 keyDown;
	u32 keyUp;
	s32 touchX;
	s32 touchY;
	s32 i;

	profilerPush(NEOPROFILE_GUI);

	//process commands
	guiSystemProcessCommand();
	
	//calculate input data
	scanKeys();
	touch = touchReadXY();
	touchX = touch.px / 8;
	touchY = touch.py / 8;

	keyRepeat = keysDown();
	keyDown = keysDown();
	keyUp = keysUp();
	for(i = 0; i < 16; i++) {
		if(keysHeld() & (1 << i)) {
			keyTime[i]++;
			if(keyTime[i] > 30 && (keyTime[i] & 7) == 0) {
				keyRepeat |= (1 << i);
			} else if(keyTime[i] > 90 && (keyTime[i] & 3) == 0) {
				keyRepeat |= (1 << i);
			} else if(keyTime[i] > 180 && (keyTime[i] & 1) == 0) {
				keyRepeat |= (1 << i);
			}
		} else {
			keyTime[i] = 0;
		}
	}
	keyRepeat &= ~KEY_TOUCH;

	//send touch events
	pTouchObj = guiFindObjectAt(touchX, touchY);
	if(keyDown & KEY_TOUCH) {
		GUIEVENT_ARGTYPE(GUIEVENT_TOUCHDOWN) args;

		args.xPos = touchX;
		args.yPos = touchY;
		pObj = pTouchObj;
		while(pObj) {
			ret = guiObjSendEvent(pObj, GUIEVENT_TOUCHDOWN, &args);
			if(ret == GUIEVENTRET_HANDLED) {
				pTouchDownObj = pObj;
				break;
			}
			pObj = pObj->pParent;
		}
		keyDown &= ~KEY_TOUCH;
	}
	if((keyUp & KEY_TOUCH)) {
		if(pTouchDownObj != NULL) {
			GUIEVENT_ARGTYPE(GUIEVENT_TOUCHUP) touchArgs;
			touchArgs.xPos = touchX;
			touchArgs.yPos = touchY;
			guiObjSendEvent(pTouchDownObj, GUIEVENT_TOUCHUP, &touchArgs);
		}
		if(pTouchObj != NULL && pTouchObj == pTouchDownObj) {
			GUIEVENT_ARGTYPE(GUIEVENT_TAP) tapArgs;
			tapArgs.xPos = touchX;
			tapArgs.yPos = touchY;
			guiObjSendEvent(pTouchDownObj, GUIEVENT_TAP, &tapArgs);
		}
		pTouchDownObj = NULL;
		keyUp &= ~KEY_TOUCH;
	}

	//send key events
	if(keyDown != 0) {
		GUIEVENT_ARGTYPE(GUIEVENT_KEYDOWN) args;
		args.keys = keyDown;
		guiBroadcastEvent(GUIEVENT_KEYDOWN, &args);
	}
	if(keyUp != 0) {
		GUIEVENT_ARGTYPE(GUIEVENT_KEYUP) args;
		args.keys = keyUp;
		guiBroadcastEvent(GUIEVENT_KEYUP, &args);
	}
	if(keyRepeat != 0) {
		GUIEVENT_ARGTYPE(GUIEVENT_KEYREPEAT) args;
		args.keys = keyRepeat;
		guiBroadcastEvent(GUIEVENT_KEYREPEAT, &args);
	}

	//send generic process event
	guiBroadcastEvent(GUIEVENT_PROCESS, NULL);

	profilerPop();
}

TGuiObject* guiObjAllocChild_r(const TGuiTypeHeader* pType, TGuiObject* pParent,
							   const TBounds* pBounds)
{
	TGuiObject* this = (TGuiObject*)guiHeapAlloc(pType->size);
	memset(this, 0, pType->size);
	if(pParent != NULL) {
		//link into parent's list of children
		this->pNextChild = pParent->pChildren;
		pParent->pChildren = this;
	} else {
		//no parent, must be root
		this->pNextChild = NULL;
	}
	//add pointer to parent
	this->pParent = pParent;
	//setup initial parameters
	this->bounds = *pBounds;
	this->pType = pType;
	this->pChildren = NULL;
	//render first frame
	this->flags = GUIOBJ_RENDERDIRTY;
	return this;
}

TGuiObject* guiObjCreateChild_r(const TGuiTypeHeader* pType, TGuiObject* pParent,
					const TBounds* pBounds)
{
	TGuiObject* this = guiObjAllocChild_r(pType, pParent, pBounds);
	guiObjSendEvent(this, GUIEVENT_CREATE, NULL);
	return this;
}
