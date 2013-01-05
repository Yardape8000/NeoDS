#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiRadioButton.h"

static void radioHandler(TGuiObject* pObj, void* arg)
{
	TGuiRadioButton* this = guiObjStaticCast((TGuiObject*)arg, TGuiRadioButton);
	TGuiRadioButton* pOther = guiObjDynamicCast(pObj, TGuiRadioButton);

	if(pOther != 0 && pOther != this) {
		if(pOther->group == this->group) {
			guiCheckboxSetChecked((TGuiCheckbox*)pOther, false);
		}
	}
}

GUIOBJ_IMPLEMENT_CHILD(TGuiCheckbox, TGuiRadioButton)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	this->group = 0;
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TGuiBorderStyle style = GUIBORDER_ROUND_NORMAL;
	TBounds bounds;
	if(this->parent.parent.state == GUIBUTTON_PRESSED) {
		style = GUIBORDER_ROUND_PRESSED;
	}
	guiObjGetGlobalBounds((TGuiObject*)this, &bounds);
	guiRenderFrame(bounds.x0, bounds.y0, 2, 2, style);
	guiRenderString(bounds.x0 + 2, bounds.y0, this->parent.parent.parent.szText);
	if(this->parent.checked) {
		guiRenderChar(bounds.x0, bounds.y0, GUICHAR_RADIO);
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_TAP)
{
	TGuiObject* pObj = (TGuiObject*)this;

	guiObjForeachChild(pObj->pParent, radioHandler, this);
	guiCheckboxSetChecked((TGuiCheckbox*)this, true);
	guiObjSendEvent((TGuiObject*)this, GUIEVENT_SELECTED, NULL);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

void guiRadioButtonSetGroup(TGuiRadioButton* this, u32 group)
{
	this->group = group;
}
