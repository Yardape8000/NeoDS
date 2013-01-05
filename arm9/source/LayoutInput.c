#include "Default.h"
#include "NeoIPC.h"
#include "NeoVideo.h"
#include "NeoConfig.h"
#include "guiBase.h"
#include "guiLabel.h"
#include "guiButton.h"
#include "guiCheckbox.h"
#include "guiStatus.h"
#include "guiConsole.h"
#include "guiMenu.h"
#include "LayoutInput.h"

GUIOBJ_IMPLEMENT_HANDLER(TGuiObject, dsKeyHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	guiObjGetGlobalBounds(this, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	guiRenderString(bounds.x0, bounds.y0 + 0,  "     A");
	guiRenderString(bounds.x0, bounds.y0 + 2,  "     B");
	guiRenderString(bounds.x0, bounds.y0 + 4,  "     X");
	guiRenderString(bounds.x0, bounds.y0 + 6,  "     Y");
	guiRenderString(bounds.x0, bounds.y0 + 8,  "     L");
	guiRenderString(bounds.x0, bounds.y0 + 10, "     R");
	guiRenderString(bounds.x0, bounds.y0 + 12, " Start");
	guiRenderString(bounds.x0, bounds.y0 + 14, "Select");
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiObject, ngKeyHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	guiObjGetGlobalBounds(this, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	guiRenderVertString(bounds.x0 + 0, bounds.y0,  "    A");
	guiRenderVertString(bounds.x0 + 2, bounds.y0,  "    B");
	guiRenderVertString(bounds.x0 + 4, bounds.y0,  "    C");
	guiRenderVertString(bounds.x0 + 6, bounds.y0,  "    D");
	guiRenderVertString(bounds.x0 + 8, bounds.y0,  "Start");
	guiRenderVertString(bounds.x0 + 10, bounds.y0, "Selct");
	guiRenderVertString(bounds.x0 + 12, bounds.y0, " Coin");
	guiRenderVertString(bounds.x0 + 14, bounds.y0, "Pause");
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, backHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePop();
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiCheckbox, keyGridHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	TGuiLayoutInput* pLayout = (TGuiLayoutInput*)guiGetRoot();
	u32 row, col;

	for(col = 0; col < 8; col++) {
		for(row = 0; row < 8; row++) {
			if(pLayout->pKeyGrid[row][col] == this) goto foundIt;
		}
	}
foundIt:
	ASSERT(row < 8);
	ASSERT(col < 8);

	if(this->checked) {
		g_neo->keyGrid[row] |= (1 << col);
	} else {
		g_neo->keyGrid[row] &= ~(1 << col);
	}
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT(TGuiLayoutInput)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	const TBounds dsKeyBounds = {{1, 7, 7, 22}};
	const TBounds ngKeyBounds = {{8, 1, 23, 6}};
	const TBounds backBounds = {{24, 21, 30, 22}};
	u32 row, col;

	this->romEnabled = g_neo->active;

	TGuiObject* pDsKeyLabel = guiObjCreate(TGuiObject, &dsKeyBounds);
	guiObjSetHandler(pDsKeyLabel, dsKeyHandler);

	TGuiObject* pNgKeyLabel = guiObjCreate(TGuiObject, &ngKeyBounds);
	guiObjSetHandler(pNgKeyLabel, ngKeyHandler);

	for(col = 0; col < 8; col++) {
		for(row = 0; row < 8; row++) {
			const TBounds boxCounds = {{8 + col * 2, 7 + row * 2, 9 + col * 2, 8 + row * 2}};
			this->pKeyGrid[row][col] = guiObjCreate(TGuiCheckbox, &boxCounds);
			guiObjSetHandler((TGuiObject*)this->pKeyGrid[row][col], keyGridHandler);
			if(g_neo->keyGrid[row] & (1 << col)) {
				guiCheckboxSetChecked(this->pKeyGrid[row][col], true);
			}
		}
	}

	TGuiButton* pBack = guiObjCreate(TGuiButton, &backBounds);
	guiLabelSetText(&pBack->parent, "Back");
	guiObjSetHandler(&pBack->parent.parent, backHandler);

	neoSystemSetEnabled(false);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_DESTROY)
{
	neoSystemSetEnabled(true);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()


