#include "Default.h"
#include "NeoVideo.h"
#include "NeoIPC.h"
#include "NeoConfig.h"
#include "guiBase.h"
#include "guiLabel.h"
#include "guiButton.h"
#include "guiMenu.h"
#include "guiStatus.h"
#include "guiCheckbox.h"
#include "guiRadioButton.h"
#include "LayoutRomSelect.h"
#include "LayoutDebug.h"
#include "LayoutInput.h"
#include "LayoutMain.h"

/*GUIOBJ_IMPLEMENT_HANDLER(TGuiCheckbox, audioEnableHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	neoAudioSetEnabled(this->checked);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()*/

GUIOBJ_IMPLEMENT_HANDLER(TGuiRadioButton, normalSizeHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	neoVideoSetSize(NEOVIDEO_NORMAL);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiRadioButton, scaledSizeHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	neoVideoSetSize(NEOVIDEO_SCALED);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiRadioButton, cpuFastHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	neoSystemSetClockDivide(2);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiRadioButton, cpuMediumHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	neoSystemSetClockDivide(3);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiObject, logoHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	guiObjGetGlobalBounds(this, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	guiRenderLogo(bounds.x0 + 1, bounds.y0 + 1);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, romSelectHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePush(TGuiLayoutRomSelect);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, screenOffHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePush(TGuiLayoutScreenOff);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, inputHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePush(TGuiLayoutInput);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, saveHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	//disable emulation to halt arm7 while we write save file
	neoSystemSetEnabled(false);
	neoSaveConfig();
	guiObjRenderDirty(guiGetRoot());
	neoSystemSetEnabled(true);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

#ifndef NEO_SHIPPING
GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, debugHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePush(TGuiLayoutDebug);
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()
#endif

GUIOBJ_IMPLEMENT(TGuiLayoutMain)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	const TBounds logoBounds = {{9, 1, 22, 5}};
	const TBounds romSelectBounds = {{1, 7, 14, 8}};
	const TBounds screenOffBounds = {{1, 10, 14, 11}};
	const TBounds inputBounds = {{17, 7, 30, 8}};
	const TBounds saveBounds = {{17, 10, 30, 11}};
	const TBounds ramBounds = {{25, 4, 30, 5}};

	const TBounds statusBounds = {{25, 1, 30, 3}};

	const TBounds videoFrameBounds = {{1, 14, 14, 21}};
	const TBounds videoLabelBounds = {{0, 0, 13, 1}};
	const TBounds normalSizeBounds = {{1, 3, 11, 4}};
	const TBounds scaledSizeBounds = {{1, 5, 11, 6}};

	//const TBounds audioFrameBounds = {{17, 7, 30, 12}};
	//const TBounds audioLabelBounds = {{0, 0, 13, 1}};
	//const TBounds audioEnableBounds = {{1, 3, 11, 4}};

	const TBounds cpuFrameBounds = {{17, 14, 30, 21}};
	const TBounds cpuLabelBounds = {{0, 0, 13, 1}};
	const TBounds cpuFastBounds = {{1, 3, 11, 4}};
	const TBounds cpuMediumBounds = {{1, 5, 11, 6}};

	TGuiObject* pLogo = guiObjCreate(TGuiObject, &logoBounds);
	TGuiButton* pRomSelect = guiObjCreate(TGuiButton, &romSelectBounds);
	TGuiButton* pScreenOff = guiObjCreate(TGuiButton, &screenOffBounds);
	TGuiButton* pInput = guiObjCreate(TGuiButton, &inputBounds);
	TGuiButton* pSave = guiObjCreate(TGuiButton, &saveBounds);
	TGuiStatus* ATTR_UNUSED pStatus = guiObjCreate(TGuiStatus, &statusBounds);

	if(systemIsSlot2Active()) {
		TGuiLabel* pRam = guiObjCreate(TGuiLabel, &ramBounds);
		guiLabelSetText(pRam, "SLOT2");
	}

	guiLabelSetText(&pRomSelect->parent, "Load Rom");
	guiLabelSetText(&pScreenOff->parent, "Screen Off");
	guiLabelSetText(&pInput->parent, "Input");
	guiLabelSetText(&pSave->parent, "Save");
	
	guiObjSetHandler(pLogo, logoHandler);
	guiObjSetHandler(&pRomSelect->parent.parent, romSelectHandler);
	guiObjSetHandler(&pScreenOff->parent.parent, screenOffHandler);
	guiObjSetHandler(&pInput->parent.parent, inputHandler);
	guiObjSetHandler(&pSave->parent.parent, saveHandler);
	
#ifndef NEO_SHIPPING
	const TBounds debugBounds = {{0, 22, 13, 23}};
	TGuiButton* pDebug = guiObjCreate(TGuiButton, &debugBounds);
	guiLabelSetText(&pDebug->parent, "Debug");
	guiObjSetHandler(&pDebug->parent.parent, debugHandler);
#endif

	//audio
	/*TGuiLabel* pAudioFrame = guiObjCreate(TGuiLabel, &audioFrameBounds);
	TGuiLabel* pAudioLabel = guiObjCreateChild(TGuiLabel, (TGuiObject*)pAudioFrame, &audioLabelBounds);
	TGuiCheckbox* pAudioEnable = guiObjCreateChild(TGuiCheckbox, (TGuiObject*)pAudioFrame, &audioEnableBounds);
	guiLabelSetText((TGuiLabel*)pAudioLabel, "Audio");
	guiLabelSetText((TGuiLabel*)pAudioEnable, "Enabled");
	guiCheckboxSetChecked(pAudioEnable, NEOIPC->audioEnabled);
	guiObjSetHandler((TGuiObject*)pAudioEnable, audioEnableHandler);*/

	//cpu
	TGuiLabel* pCpuFrame = guiObjCreate(TGuiLabel, &cpuFrameBounds);
	TGuiLabel* pCpuLabel = guiObjCreateChild(TGuiLabel, (TGuiObject*)pCpuFrame, &cpuLabelBounds);
	this->pFastClock = guiObjCreateChild(TGuiRadioButton, (TGuiObject*)pCpuFrame, &cpuFastBounds);
	this->pMediumClock = guiObjCreateChild(TGuiRadioButton, (TGuiObject*)pCpuFrame, &cpuMediumBounds);

	guiLabelSetText((TGuiLabel*)pCpuLabel, "CPU Clock");
	guiLabelSetText((TGuiLabel*)this->pFastClock, "Normal");
	guiLabelSetText((TGuiLabel*)this->pMediumClock, "Slower");
	guiObjSetHandler((TGuiObject*)this->pFastClock, cpuFastHandler);
	guiObjSetHandler((TGuiObject*)this->pMediumClock, cpuMediumHandler);
	
	//video
	TGuiLabel* pVideoFrame = guiObjCreate(TGuiLabel, &videoFrameBounds);
	TGuiLabel* pVideoLabel = guiObjCreateChild(TGuiLabel, (TGuiObject*)pVideoFrame, &videoLabelBounds);
	this->pNormalSize = guiObjCreateChild(TGuiRadioButton, (TGuiObject*)pVideoFrame, &normalSizeBounds);
	this->pScaledSize = guiObjCreateChild(TGuiRadioButton, (TGuiObject*)pVideoFrame, &scaledSizeBounds);
	guiLabelSetText((TGuiLabel*)pVideoLabel, "Video");
	guiLabelSetText((TGuiLabel*)this->pNormalSize, "Normal");
	guiLabelSetText((TGuiLabel*)this->pScaledSize, "Scaled");
	guiObjSetHandler((TGuiObject*)this->pNormalSize, normalSizeHandler);
	guiObjSetHandler((TGuiObject*)this->pScaledSize, scaledSizeHandler);
	
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_ENABLE)
{
	guiCheckboxSetChecked((TGuiCheckbox*)this->pNormalSize, false);
	guiCheckboxSetChecked((TGuiCheckbox*)this->pScaledSize, false);
	guiCheckboxSetChecked((TGuiCheckbox*)this->pMediumClock, false);
	guiCheckboxSetChecked((TGuiCheckbox*)this->pFastClock, false);

	switch(neoVideoGetSize()) {
	case NEOVIDEO_NORMAL:
		guiCheckboxSetChecked((TGuiCheckbox*)this->pNormalSize, true);
		break;
	case NEOVIDEO_SCALED:
		guiCheckboxSetChecked((TGuiCheckbox*)this->pScaledSize, true);
		break;
	default:
		break;
	}
	switch(g_neo->cpuClockDivide) {
	case 3: guiCheckboxSetChecked((TGuiCheckbox*)this->pMediumClock, true); break;
	default: guiCheckboxSetChecked((TGuiCheckbox*)this->pFastClock, true); break;
	}
}

GUIOBJ_IMPLEMENT_END()



GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, screenOnHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePop();
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT(TGuiLayoutScreenOff)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	const TBounds buttonBounds = {{0, 0, 32, 24}};
	TGuiButton* pButton = guiObjCreate(TGuiButton, &buttonBounds);
	guiObjSetHandler(&pButton->parent.parent, screenOnHandler);

	this->saveMode = SUB_DISPLAY_CR;
	SUB_DISPLAY_CR = MODE_0_2D;
	neoIPCSendCommand(NEOARM7_BACKLIGHTOFF);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_DESTROY)
{
	SUB_DISPLAY_CR = this->saveMode;
	neoIPCSendCommand(NEOARM7_BACKLIGHTON);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()
