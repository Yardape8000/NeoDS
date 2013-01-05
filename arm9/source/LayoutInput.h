#ifndef _LAYOUT_INPUT_H
#define _LAYOUT_INPUT_H

#include "guiBase.h"
#include "guiCheckbox.h"

GUIOBJ_DECLARE(TGuiLayoutInput)
	TGuiCheckbox* pKeyGrid[8][8];
	bool romEnabled;
};

#endif
