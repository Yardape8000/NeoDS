#ifndef _NEO_AUDIO_H
#define _NEO_AUDIO_H

//typedef enum _TNeoAudioEventType {
//	NEOAUDIO_VOLUMEPAN,
//} TNeoAudioEventType;

#define SOUND_VOLUME_MASK 0x7f
#define SOUND_PAN_MASK (0x7f << 16)
#define SOUND_VOLUMEPAN_MASK (SOUND_VOLUME_MASK|SOUND_PAN_MASK)

void neoAudioInit();
void neoAudioReset();
void neoAudioShutdown();
void neoAudioEventHandler(); //timer3 intr
u32 neoAudioGetTimestamp();
void neoAudioPause();
void neoAudioResume();
void neoAudioUpdate();

//events are processed by the arm7 1 audio frame in the future (triggered by timer)
void neoAudioBufferWrite16(vu16* reg, u16 value, u16 mask);
void neoAudioBufferWrite32(vu32* reg, u32 value, u32 mask);

//commands are processed by the arm9 1 audio frame in the future

#endif
