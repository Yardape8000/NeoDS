#ifndef _GUI_RADIOBUTTON_H
#define _GUI_RADIOBUTTON_H

#include "guiCheckbox.h"

GUIOBJ_DECLARE_CHILD(TGuiCheckbox, TGuiRadioButton)
	u32 group;
};

void guiRadioButtonSetGroup(TGuiRadioButton* this, u32 group);

#endif
