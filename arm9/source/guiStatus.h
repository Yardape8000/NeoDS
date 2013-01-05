#ifndef _GUI_STATUS_H
#define _GUI_STATUS_H

#include "guiBase.h"

GUIOBJ_DECLARE(TGuiStatus)
	u32 lastFps;
	u32 lastSpriteCount;
	char szFps[8];
	char szSpriteCount[8];
};


#endif
