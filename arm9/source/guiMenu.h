#ifndef _GUI_MENU_H
#define _GUI_MENU_H

#include "guiBase.h"

typedef struct _TGuiMenuItem {
	char szText[GUI_MAX_TEXT];
} TGuiMenuItem;

GUIOBJ_DECLARE(TGuiMenu)
	TGuiMenuItem* pItem;
	s32 selected;
	s32 top;
	s32 itemCount;
	s32 itemMax;
};

TGuiMenu* guiMenuCreateChild(TGuiObject* pParent, s32 itemMax, const TBounds* pBounds);
static inline TGuiMenu* guiMenuCreat(s32 itemMax, const TBounds* pBounds)
{
	return guiMenuCreateChild(NULL, itemMax, pBounds);
}

void guiMenuAddItem(TGuiMenu* this, const char* szText);

static inline const TGuiMenuItem* guiMenuGetSelected(const TGuiMenu* this)
{
	if(this->itemCount == 0) {
		return NULL;
	}
	return &this->pItem[this->selected];
}

#endif
