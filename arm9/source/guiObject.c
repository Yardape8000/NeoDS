#include "Default.h"
#include "guiBase.h"

static TGuiEventReturn GUITYPE_HANDLER(TGuiObject) GUIHANDLER_ARGS
{
	return GUIEVENTRET_NOTHANDLED;
}

const TGuiTypeHeader GUITYPE_HEADER(TGuiObject) = {
	GUITYPE_HANDLER(TGuiObject),
	NULL,
	sizeof(TGuiObject),
	"TGuiObject",
};

bool guiObjIsType_r(const TGuiObject* this, const TGuiTypeHeader* pType)
{
	const TGuiTypeHeader* pMyType = this->pType;
	while(pMyType) {
		if(pMyType == pType) return true;
		pMyType = pMyType->pParent;
	}
	return false;
}

void guiObjGetGlobalBounds(const TGuiObject* this, TBounds* pBounds)
{
	s32 w = this->bounds.x1 - this->bounds.x0 + 1;
	s32 h = this->bounds.y1 - this->bounds.y0 + 1;
	s32 x = this->bounds.x0;
	s32 y = this->bounds.y0;
	const TGuiObject* pParent = this->pParent;
	while(pParent) {
		x += pParent->bounds.x0;
		y += pParent->bounds.y0;
		pParent = pParent->pParent;
	}
	pBounds->x0 = x;
	pBounds->y0 = y;
	pBounds->x1 = x + w - 1;
	pBounds->y1 = y + h - 1;
}

TGuiEventReturn guiObjSendEvent(TGuiObject* this, TGuiEventID e, void* arg)
{
	const TGuiTypeHeader* pType = this->pType;
	TGuiEventReturn ret = GUIEVENTRET_NOTHANDLED;

	//special actions hardwired to certain events
	switch(e) {
	case GUIEVENT_RENDER:
		if(!(this->flags & GUIOBJ_RENDERDIRTY)) {
			//don't do anything if not dirty
			return GUIEVENTRET_HANDLED;
		}
		this->flags &= ~GUIOBJ_RENDERDIRTY;
		if(this->flags & GUIOBJ_RENDERCLEAR) {
			//clear render area if needed
			TBounds bounds;
			guiObjGetGlobalBounds(this, &bounds);
			guiRenderClearBounds(&bounds);
			this->flags &= ~GUIOBJ_RENDERCLEAR;
		}
		break;
	case GUIEVENT_ENABLE:
		guiObjRenderDirty(this);
		break;
	default:
		break;
	}

	//try local handler if it exists
	if(this->handler) {
		ret = this->handler(this, e, arg);
	}
	//run through all base type handlers until one handles the event,
	//or we run out of parents
	while(ret == GUIEVENTRET_NOTHANDLED && pType != NULL) {
		ret = pType->handler(this, e, arg);
		pType = pType->pParent;
	}
	return ret;
}

static void guiObjRenderDirtyChildren(TGuiObject* this)
{
	//im already dirty
	if(this->flags & GUIOBJ_RENDERDIRTY) {
		return;
	}
	//mark me as dirty
	this->flags |= GUIOBJ_RENDERDIRTY;
	//mark all children as dirty
	TGuiObject* pChild = this->pChildren;
	while(pChild) {
		guiObjRenderDirtyChildren(pChild);
		pChild = pChild->pNextChild;
	}
}

void guiObjRenderDirty(TGuiObject* this)
{
	if(this->flags & GUIOBJ_RENDERDIRTY) {
		return;
	}
	//mark me as dirty
	this->flags |= GUIOBJ_RENDERDIRTY;
	//mark parent as dirty
	if(guiObjIsRoot(this->pParent)) {
		//obj is top level
		this->flags |= GUIOBJ_RENDERCLEAR;
	} else {
		guiObjRenderDirty(this->pParent);
	}
	//mark children as dirty
	TGuiObject* pChild = this->pChildren;
	while(pChild) {
		guiObjRenderDirtyChildren(pChild);
		pChild = pChild->pNextChild;
	}
}

TGuiEventHandler guiObjSetHandler(TGuiObject* this, TGuiEventHandler handler)
{
	TGuiEventHandler old = this->handler;
	this->handler = handler;
	return old;
}

void guiObjForeachChild(const TGuiObject* this, TGuiObjIterator iterator, void* arg)
{
	TGuiObject* pChild = this->pChildren;
	while(pChild) {
		TGuiObject* pNext = pChild->pNextChild;
		iterator(pChild, arg);
		pChild = pNext;
	}
}
