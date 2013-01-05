#include "Default.h"
#include "NeoIPC.h"
#include "NeoSystem.h"
#include "guiBase.h"
#include "guiLabel.h"
#include "guiButton.h"
#include "guiCheckbox.h"
#include "guiMenu.h"
#include "LayoutRomSelect.h"

GUIOBJ_IMPLEMENT_HANDLER(TGuiMenu, romMenuHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	TGuiLayoutRomSelect* pLayout = (TGuiLayoutRomSelect*)guiGetRoot();
	const TGuiMenuItem* pItem = guiMenuGetSelected(this);
	//only set when rom is selected
	NEOIPC->globalAudioEnabled = pLayout->globalAudioEnabled;
	bool openOk = neoSystemOpen(pItem->szText);
	if(openOk) {
		pLayout->romEnabled = true;
		guiFramePop();
	}
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, cancelHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePop();
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiCheckbox, audioEnableHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	TGuiLayoutRomSelect* pLayout = (TGuiLayoutRomSelect*)guiGetRoot();
	pLayout->globalAudioEnabled = this->checked;
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT(TGuiLayoutRomSelect)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	const TBounds headerBounds = {{0, 0, 29, 1}};
	const TBounds frameBounds = {{1, 1, 30, 19}};
	const TBounds menuBounds = {{2, 3, 27, 17}};
	
	const TBounds cancelBounds = {{1, 21, 14, 22}};
	const TBounds audioBounds = {{17, 21, 30, 22}};

	TGuiLabel* pFrame;
	TGuiMenu* pMenu;
	u32 i;

	this->globalAudioEnabled = NEOIPC->globalAudioEnabled;
	this->romEnabled = g_neo->active;

	pFrame = guiObjCreate(TGuiLabel, &frameBounds);
	pMenu = guiMenuCreateChild(&pFrame->parent, neoSystemGetRomCount(), &menuBounds);
	for(i = 0; i < neoSystemGetRomCount(); i++) {
		guiMenuAddItem(pMenu, neoSystemGetRomName(i));
	}

	guiObjSetHandler(&pMenu->parent, romMenuHandler);

	TGuiLabel* pLabel = guiObjCreateChild(TGuiLabel, (TGuiObject*)pFrame, &headerBounds);
	guiLabelSetText(pLabel, "Select Rom");

	TGuiButton* pCancel = guiObjCreate(TGuiButton, &cancelBounds);
	guiLabelSetText(&pCancel->parent, "Cancel");
	guiObjSetHandler(&pCancel->parent.parent, cancelHandler);

	TGuiCheckbox* pAudioEnable = guiObjCreate(TGuiCheckbox, &audioBounds);
	guiLabelSetText((TGuiLabel*)pAudioEnable, "Audio");
	guiCheckboxSetChecked(pAudioEnable, NEOIPC->globalAudioEnabled);
	guiObjSetHandler((TGuiObject*)pAudioEnable, audioEnableHandler);

	//neoAudioSetEnabled(false);
	neoSystemSetEnabled(false);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_DESTROY)
{
	//neoAudioSetEnabled(this->audioEnabled);
	neoSystemSetEnabled(true);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()
