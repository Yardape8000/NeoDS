#ifndef _GUI_LABEL_H
#define _GUI_LABEL_H

#include "guiBase.h"

GUIOBJ_DECLARE(TGuiLabel)
	char szText[GUI_MAX_TEXT];
};

void guiLabelSetText(TGuiLabel* this, const char* szText);
void guiLabelSetTextv(TGuiLabel* this, const char* szFormat, ...);

#endif
