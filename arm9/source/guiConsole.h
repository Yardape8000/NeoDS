#ifndef _GUI_CONSOLE_H
#define _GUI_CONSOLE_H

#include "guiBase.h"

GUIOBJ_DECLARE(TGuiConsole)
	u32 lastCount;
};

void guiConsoleDump();
void guiConsoleLog(const char* szText);
void guiConsoleLogf(const char* szText, ...);
void guiConsoleLogfv(const char* szFormat, va_list v);

#endif
