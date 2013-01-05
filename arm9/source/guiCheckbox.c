#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiCheckbox.h"

GUIOBJ_IMPLEMENT_CHILD(TGuiButton, TGuiCheckbox)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	this->checked = false;
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TGuiBorderStyle style = GUIBORDER_NORMAL;
	TBounds bounds;
	if(this->parent.state == GUIBUTTON_PRESSED) {
		style = GUIBORDER_PRESSED;
	}
	guiObjGetGlobalBounds((TGuiObject*)this, &bounds);
	guiRenderFrame(bounds.x0, bounds.y0, 2, 2, style);
	guiRenderString(bounds.x0 + 2, bounds.y0, this->parent.parent.szText);
	if(this->checked) {
		guiRenderChar(bounds.x0, bounds.y0, GUICHAR_CHECK);
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_TAP)
{
	this->checked = !this->checked;
	guiObjRenderDirty((TGuiObject*)this);
	guiObjSendEvent((TGuiObject*)this, GUIEVENT_SELECTED, NULL);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

void guiCheckboxSetChecked(TGuiCheckbox* this, bool checked)
{
	this->checked = checked;
	guiObjRenderDirty((TGuiObject*)this);
}
