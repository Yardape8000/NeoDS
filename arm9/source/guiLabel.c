#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiLabel.h"

GUIOBJ_IMPLEMENT(TGuiLabel)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_CREATE)
{
	this->szText[0] = 0;
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	guiObjGetGlobalBounds(&this->parent, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	guiRenderString(bounds.x0, bounds.y0, this->szText);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

void guiLabelSetText(TGuiLabel* this, const char* szText)
{
	strncpy(this->szText, szText, GUI_MAX_TEXT);
	guiObjRenderDirty(&this->parent);
}

void guiLabelSetTextv(TGuiLabel* this, const char* szFormat, ...)
{
	va_list v;
	va_start(v, szFormat);
	//vsniprintf(this->szText, GUI_MAX_TEXT, szFormat, v);
	neoVsnprintf(this->szText, GUI_MAX_TEXT, szFormat, v);
	va_end(v);
}
