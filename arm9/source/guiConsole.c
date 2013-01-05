#include "Default.h"
#include "guiBase.h"
#include "guiRender.h"
#include "guiConsole.h"

#define LOG_WIDTH 32
#define LOG_HEIGHT 24

static char g_log[LOG_WIDTH * LOG_HEIGHT] ALIGN(32);
static u32 g_line = 0;
static u32 g_logCount = 0;

GUIOBJ_IMPLEMENT(TGuiConsole)

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_RENDER)
{
	const s32 height = guiObjGetHeight(&this->parent) - 1;
	s32 line = g_line;
	TBounds bounds;
	s32 y;

	guiObjGetGlobalBounds(&this->parent, &bounds);
	guiRenderFrameBounds(&bounds, GUIBORDER_NORMAL);
	for(y = 0; y < height; y++) {
		line--;
		if(line < 0) line += LOG_HEIGHT;
	}
	for(y = 0; y < height; y++) {
		ASSERT(line >= 0 && line < LOG_HEIGHT);
		guiRenderStringn(bounds.x0, bounds.y0 + y, LOG_WIDTH - 1, &g_log[line * LOG_WIDTH]);
		line++;
		if(line >= LOG_HEIGHT) line -= LOG_HEIGHT;
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJHANDLER_IMPLEMENT(GUIEVENT_PROCESS)
{
	if(this->lastCount != g_logCount) {
		guiObjRenderDirty(&this->parent);
		this->lastCount = g_logCount;
	}
	GUIOBJHANDLER_HANDLE();
}

GUIOBJ_IMPLEMENT_END()

void guiConsoleDump()
{
	s32 line = g_line;
	s32 y;

	guiRenderClear();

	for(y = 0; y < GUI_HEIGHT - 1; y++) {
		line--;
		if(line < 0) line += LOG_HEIGHT;
	}
	for(y = 0; y < GUI_HEIGHT - 1; y++) {
		guiRenderStringn(0, y, LOG_WIDTH - 1, &g_log[line * LOG_WIDTH]);
		line++;
		if(line >= LOG_HEIGHT) line -= LOG_HEIGHT;
	}
}

void guiConsoleLog(const char* szText)
{
	while(*szText) {
		char* pDst = &g_log[g_line * LOG_WIDTH];
		s32 lineCount = LOG_WIDTH - 1;

		while(*szText && lineCount > 0) {
			*pDst++ = *szText++;
			lineCount--;
		}
		if(lineCount > 0) {
			*pDst = 0;
		}
		g_line++;
		if(g_line >= LOG_HEIGHT) {
			g_line = 0;
		}
	}
	g_logCount++;
}

extern int neoVsnprintf (char *str, size_t count, const char *fmt, va_list arg);

void guiConsoleLogfv(const char* szFormat, va_list v)
{
	char szBuffer[64];
	//vsniprintf(szBuffer, sizeof(szBuffer), szFormat, v);
	neoVsnprintf(szBuffer, sizeof(szBuffer), szFormat, v);
	guiConsoleLog(szBuffer);
}

void guiConsoleLogf(const char* szFormat, ...)
{
	va_list v;
	va_start(v, szFormat);
	guiConsoleLogfv(szFormat, v);
	va_end(v);
}
