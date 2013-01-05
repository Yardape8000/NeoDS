#ifndef _GUI_BUTTON_H
#define _GUI_BUTTON_H

#include "guiLabel.h"

typedef enum _TGuiButtonState {
	GUIBUTTON_NORMAL,
	GUIBUTTON_PRESSED,
} TGuiButtonState;

GUIOBJ_DECLARE_CHILD(TGuiLabel, TGuiButton)
	TGuiButtonState state;
};

#endif
