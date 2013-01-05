#ifndef _GUI_CHECKBOX_H
#define _GUI_CHECKBOX_H

#include "guiButton.h"

GUIOBJ_DECLARE_CHILD(TGuiButton, TGuiCheckbox)
	bool checked;
};

void guiCheckboxSetChecked(TGuiCheckbox* this, bool checked);

#endif
