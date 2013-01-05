#ifndef _LAYOUT_MAIN_H
#define _LAYOUT_MAIN_H

#include "guiBase.h"

struct _TGuiRadioButton;

GUIOBJ_DECLARE(TGuiLayoutMain)
	struct _TGuiRadioButton* pNormalSize;
	struct _TGuiRadioButton* pScaledSize;
	struct _TGuiRadioButton* pFastClock;
	struct _TGuiRadioButton* pMediumClock;
};

GUIOBJ_DECLARE(TGuiLayoutScreenOff)
	u32 saveMode;
};

#endif
