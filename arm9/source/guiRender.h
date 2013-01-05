#ifndef _GUI_RENDER_H
#define _GUI_RENDER_H

#define GUI_WIDTH 32
#define GUI_HEIGHT 24

typedef enum _TGuiBorderStyle {
	GUIBORDER_NORMAL,
	GUIBORDER_PRESSED,
	GUIBORDER_ROUND_NORMAL,
	GUIBORDER_ROUND_PRESSED,
} TGuiBorderStyle;

typedef enum _TGuiCharacters {
	GUICHAR_NULL,
	GUICHAR_A,
	GUICHAR_B,
	GUICHAR_START,
	GUICHAR_SELECT,
	GUICHAR_RIGHT,
	GUICHAR_LEFT,
	GUICHAR_UP,
	GUICHAR_DOWN,
	GUICHAR_R,
	GUICHAR_L,
	GUICHAR_X,
	GUICHAR_Y,
	GUICHAR_CHECK,
	GUICHAR_RADIO,
} TGuiCharacters;

void guiRenderInit();
void guiRenderLogo(s32 x, s32 y);
void guiRenderFrameBounds(const TBounds* pBounds, TGuiBorderStyle style);
void guiRenderFrame(s32 x, s32 y, s32 w, s32 h, TGuiBorderStyle style);
void guiRenderString(s32 x, s32 y, const char* szString);
void guiRenderVertString(s32 x, s32 y, const char* szString);
void guiRenderChar(s32 x, s32 y, char c);
void guiRenderStringn(s32 x, s32 y, u32 n, const char* szString);
void guiRenderClearBounds(const TBounds* pBounds);
void guiRenderClear();

#endif
