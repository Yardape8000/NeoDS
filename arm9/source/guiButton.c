#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiButton.h"

GUIOBJ_IMPLEMENT_CHILD(TGuiLabel, TGuiButton)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	this->state = GUIBUTTON_NORMAL;
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_TOUCHDOWN)
{
	this->state = GUIBUTTON_PRESSED;
	guiObjRenderDirty(&this->parent.parent);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_TOUCHUP)
{
	this->state = GUIBUTTON_NORMAL;
	guiObjRenderDirty(&this->parent.parent);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TGuiBorderStyle style = GUIBORDER_NORMAL;
	TBounds bounds;
	if(this->state == GUIBUTTON_PRESSED) {
		style = GUIBORDER_PRESSED;
	}
	guiObjGetGlobalBounds(&this->parent.parent, &bounds);
	guiRenderFrameBounds(&bounds, style);
	guiRenderString(bounds.x0, bounds.y0, this->parent.szText);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_TAP)
{
	guiObjSendEvent((TGuiObject*)this, GUIEVENT_SELECTED, NULL);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

