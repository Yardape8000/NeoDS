#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiMenu.h"

GUIOBJ_IMPLEMENT(TGuiMenu)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	s32 yPos;
	s32 i;

	guiObjGetGlobalBounds(&this->parent, &bounds);
	yPos = bounds.y0;
	i = this->top;

	while(yPos < bounds.y1 && i < this->itemCount) {
		if(i == this->selected) {
			guiRenderFrame(bounds.x0, yPos, bounds.x1 - bounds.x0 + 1, 2, GUIBORDER_PRESSED);
		}
		guiRenderString(bounds.x0, yPos, this->pItem[i].szText);
		yPos += 2;
		i++;
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_KEYREPEAT)
{
	s32 height = (guiObjGetHeight(&this->parent) - 1) / 2;
	if(arg->keys & KEY_DOWN) {
		this->selected++;
		if(this->selected >= this->itemCount) {
			this->selected = 0;
		}
		guiObjRenderDirty(&this->parent);
	}
	if(arg->keys & KEY_UP) {
		this->selected--;
		if(this->selected < 0) {
			this->selected = this->itemCount - 1;
		}
		guiObjRenderDirty(&this->parent);
	}
	if(this->selected < this->top) {
		this->top = this->selected;
	} else if(this->selected >= this->top + height) {
		this->top = this->selected - height + 1;
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_KEYUP)
{
	if(arg->keys & KEY_START) {
		guiObjSendEvent(&this->parent, GUIEVENT_SELECTED, NULL);
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

TGuiMenu* guiMenuCreateChild(TGuiObject* pParent, s32 itemMax, const TBounds* pBounds)
{
	TGuiMenu* this = guiObjCreateChild(TGuiMenu, pParent, pBounds);
	this->pItem = (TGuiMenuItem*)guiHeapAlloc(sizeof(TGuiMenuItem) * itemMax);
	this->itemMax = itemMax;
	this->itemCount = 0;
	this->selected = 0;
	this->top = 0;
	return this;
}

void guiMenuAddItem(TGuiMenu* this, const char* szText)
{
	ASSERTMSG(this->itemCount < this->itemMax, "guiMenuAddItem: too many items (%d/%d)",
		this->itemCount, this->itemMax);
	TGuiMenuItem* pItem = &this->pItem[this->itemCount];
	strncpy(pItem->szText, szText, GUI_MAX_TEXT);
	this->itemCount++;
	guiObjRenderDirty(&this->parent);
}
