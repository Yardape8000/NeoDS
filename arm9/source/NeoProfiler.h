#ifndef _NEO_PROFILER_H
#define _NEO_PROFILER_H

typedef enum _TNeoProfileSection {
	NEOPROFILE_ROOT,
	NEOPROFILE_CPU,
	NEOPROFILE_VIDEO_SPRITES,
	NEOPROFILE_VIDEO_FIXED,
	NEOPROFILE_AUDIO,
	NEOPROFILE_AUDIO_ADPCMA,
	NEOPROFILE_AUDIO_ADPCMB,
	NEOPROFILE_ROMREAD,
	NEOPROFILE_AUDIOWAIT,
	NEOPROFILE_GUI,

	NEOPROFILE_SECTION_COUNT
} TNeoProfileSection;

#ifdef NEO_SHIPPING
#define profilerPush(x) ((void)0)
#define profilerPop() ((void)0)
#else
void profilerPush(TNeoProfileSection section);
void profilerPop();
#endif

#endif
