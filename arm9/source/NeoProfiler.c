#include "Default.h"
#include "NeoProfiler.h"

#ifndef NEO_SHIPPING

#define PROFILER_STACK_DEPTH 16

static TNeoProfileSection g_stack[PROFILER_STACK_DEPTH];
static s32 g_stackPos = 0;

/*
NEOPROFILE_ROOT,
NEOPROFILE_CPU,
NEOPROFILE_VIDEO_SPRITES,
NEOPROFILE_VIDEO_FIXED,
NEOPROFILE_AUDIO,
NEOPROFILE_AUDIO_ADPCMA,
NEOPROFILE_AUDIO_ADPCMB,
NEOPROFILE_ROMREAD,
NEOPROFILE_AUDIOWAIT,
NEOPROFILE_GUI
	*/
static const u16 g_sectionColor[NEOPROFILE_SECTION_COUNT] = {
	RGB15(0, 0, 0),
	RGB15(31, 0, 0),
	RGB15(0, 31, 0),
	RGB15(0, 0, 31),
	RGB15(15, 15, 15),
	RGB15(31, 0, 31),
	RGB15(0, 31, 31),
	RGB15(31, 31, 0),
	RGB15(31, 31, 31),
	RGB15(8, 0, 8),
};

void profilerPush(TNeoProfileSection section)
{
	ASSERT(g_stackPos < PROFILER_STACK_DEPTH);
	g_stack[g_stackPos++] = section;
	BG_PALETTE_SUB[0] = g_sectionColor[section];

}

void profilerPop()
{
	ASSERT(g_stackPos > 0);
	g_stackPos--;
	BG_PALETTE_SUB[0] = g_sectionColor[g_stack[g_stackPos]];
}

#endif
