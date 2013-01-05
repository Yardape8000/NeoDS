#ifndef NEO_SHIPPING

#include "Default.h"
#include "NeoIPC.h"
#include "NeoVideo.h"
#include "guiBase.h"
#include "guiLabel.h"
#include "guiButton.h"
#include "guiStatus.h"
#include "guiConsole.h"
#include "guiMenu.h"
#include "LayoutDebug.h"

GUIOBJ_IMPLEMENT_HANDLER(TGuiButton, backHandler)
GUIOBJHANDLER_IMPLEMENT(GUIEVENT_SELECTED)
{
	guiFramePop();
	GUIOBJHANDLER_HANDLE();
}
GUIOBJ_IMPLEMENT_END()

GUIOBJ_IMPLEMENT(TGuiLayoutDebug)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	const TBounds statusBounds = {{27, 0, 31, 2}};
	const TBounds consoleBounds = {{0, 3, 31, 19}};
	const TBounds backBounds = {{1, 21, 14, 22}};

	TGuiConsole* ATTR_UNUSED pConsole = guiObjCreate(TGuiConsole, &consoleBounds);
	TGuiButton* pBack = guiObjCreate(TGuiButton, &backBounds);
	TGuiStatus* ATTR_UNUSED pStatus = guiObjCreate(TGuiStatus, &statusBounds);

	guiLabelSetText(&pBack->parent, "Back");

	guiObjSetHandler(&pBack->parent.parent, backHandler);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

#endif

