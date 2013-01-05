#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiStatus.h"

//from NeoSprite
extern u32 g_spriteCount;

GUIOBJ_IMPLEMENT(TGuiStatus)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	TBounds bounds;
	guiObjGetGlobalBounds(&this->parent, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	guiRenderString(bounds.x0, bounds.y0, this->szFps);
	guiRenderString(bounds.x0, bounds.y0 + 1, this->szSpriteCount);
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_PROCESS)
{
	if(this->lastSpriteCount != g_spriteCount || this->lastFps != g_currentFps) {
		u32 tens = g_currentFps / 10;
		u32 ones = g_currentFps % 10;
		u32 hundreds;
		u32 thousands;
		u32 rem;
		
		
		this->szFps[0] = ' ';
		this->szFps[1] = ' ';
		this->szFps[2] = (u16)('0' + tens) | 0xf000;
		this->szFps[3] = (u16)('0' + ones) | 0xf000;
		this->szFps[4] = 0;

		thousands = g_spriteCount / 1000;
		rem = g_spriteCount % 1000;
		hundreds = rem / 100;
		rem = rem % 100;
		tens = rem / 10;
		ones = rem % 10;

		this->szSpriteCount[0] = (u16)('0' + thousands) | 0xf000;
		this->szSpriteCount[1] = (u16)('0' + hundreds) | 0xf000;
		this->szSpriteCount[2] = (u16)('0' + tens) | 0xf000;
		this->szSpriteCount[3] = (u16)('0' + ones) | 0xf000;
		this->szSpriteCount[4] = 0;
		guiObjRenderDirty(&this->parent);
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()
