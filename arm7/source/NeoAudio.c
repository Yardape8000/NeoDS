#include "nds.h"
#include <string.h>
#include "NeoSystem7.h"
#include "NeoIPC.h"
#include "NeoAudioStream.h"
#include "NeoAudio.h"

#define NEO_EVENT_QUEUE_SIZE 256

typedef struct _TNeoAudioEvent {
	u32 value;
	u16 timeStamp;
	u16 audioFrame;
	u8 offset;
	u8 bits;
} TNeoAudioEvent;

#define AUDIO_IO_BASE 0x04000400
#define AUDIO_IO_TOP 0x04000500

static TNeoAudioEvent g_audioEvent[NEO_EVENT_QUEUE_SIZE];
static u32 g_eventSent;
static volatile u32 g_eventProcessed;
static u8 g_audioIOMap[256] ALIGN(4);
static u32 g_lastTimer = 0;

static void neoAudioDoWrite(const TNeoAudioEvent* pEvent)
{
	/*u32 value;

	//process event
	switch(type) {
	case NEOAUDIO_VOLUMEPAN:
		value = SCHANNEL_CR(channel) & ~0x7f & ~(0x7f << 16);
		SCHANNEL_CR(channel) = value | param;
		break;
	}*/
	if(pEvent->bits == 16) {
		vu16* pReg = (vu16*)(AUDIO_IO_BASE + pEvent->offset);
		*pReg = (u16)pEvent->value;
	} else {
		vu32* pReg = (vu32*)(AUDIO_IO_BASE + pEvent->offset);
		*pReg = pEvent->value;
	}
}

void neoAudioEventHandler()
{
	s32 eventIndex = g_eventProcessed;
	s32 eventCount = (s32)g_eventSent - eventIndex;

	TIMER_CR(3) = 0;

	if(eventCount < 0) eventCount += NEO_EVENT_QUEUE_SIZE;
	
	while(1) {
		const TNeoAudioEvent* pEvent = &g_audioEvent[eventIndex];

		neoAudioDoWrite(pEvent);
	
		eventCount--;
		eventIndex++;
		if(eventIndex >= NEO_EVENT_QUEUE_SIZE) eventIndex = 0;

		if(eventCount == 0) {
			//no more events
			break;
		}
		
		const TNeoAudioEvent* pNextEvent = &g_audioEvent[eventIndex];
		s32 deltaT = (s32)pNextEvent->timeStamp - (s32)pEvent->timeStamp;
		if(pNextEvent->audioFrame > pEvent->audioFrame) {
			deltaT += (pNextEvent->audioFrame - pEvent->audioFrame) *
				NEO_ADPCMA_STREAM_SIZE;
		}
		//ASSERT(deltaT >= 0);
		if(deltaT > 0) {
			//next event doesn't happen yet...
			//program timer to generate event at right time
			TIMER_DATA(3) = 65536 - deltaT;
			TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE | TIMER_IRQ_REQ;
			break;
		}
	}
	g_eventProcessed = eventIndex;
}

static inline u32 neoAudioIOMapRead32(vu32* reg)
{
	return *(u32*)(g_audioIOMap + (u32)reg - AUDIO_IO_BASE);
}

static inline u16 neoAudioIOMapRead16(vu16* reg)
{
	return *(u16*)(g_audioIOMap + (u32)reg - AUDIO_IO_BASE);
}

static inline void neoAudioIOMapWrite32(vu32* reg, u32 value)
{
	*(u32*)(g_audioIOMap + (u32)reg - AUDIO_IO_BASE) = value;
}

static inline void neoAudioIOMapWrite16(vu16* reg, u16 value)
{
	*(u16*)(g_audioIOMap + (u32)reg - AUDIO_IO_BASE) = value;
}

static inline void neoAudioIOWrite32(vu32* reg, u32 value)
{
	neoAudioIOMapWrite32(reg, value);
	*reg = value;
}

static inline void neoAudioIOWrite16(vu16* reg, u16 value)
{
	neoAudioIOMapWrite16(reg, value);
	*reg = value;
}

void neoAudioInit()
{
	powerON(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
	neoAudioReset();
}

void neoAudioShutdown()
{
	u32 i;

	NEOIPC->audioStreamCount = 0;
	g_eventSent = 0;
	g_eventProcessed = 0;
	g_lastTimer = 0;

	//disable all timer and audio registers
	for(i = 0; i < 4; i++) {
		TIMER_CR(i) = 0;
	}
	for(i = 0; i < 16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_REPEAT_POINT(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}
	for(i = 0; i < 256; i++) {
		g_audioIOMap[i] = 0;
	}
}

void neoAudioReset()
{
	u16 enabled = REG_IME;
	u32 i;

	//disable interrupts while we set everything up
	REG_IME = 0;

	neoAudioShutdown();

	//timer 0 and 1 drive arm9 streaming updates
	TIMER_DATA(0) = SOUND_FREQ(NEO_ADPCMA_RATE) * 2;
	TIMER_CR(0) = TIMER_ENABLE;
	
	TIMER_DATA(1) = 65536 - NEO_ADPCMA_STREAM_SIZE;
	TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE;

	//timer 2 and 3 drive arm7 audio events (in sync with arm9 streaming)
	TIMER_DATA(2) = SOUND_FREQ(NEO_ADPCMA_RATE) * 2;
	TIMER_CR(2) = TIMER_ENABLE;

	TIMER_DATA(3) = 0;//65536 - NEO_ADPCMA_STREAM_SIZE;
	TIMER_CR(3) = 0;//TIMER_ENABLE | TIMER_CASCADE | TIMER_IRQ_REQ;

	for(i = 0; i < 7; i++) {
		//set up 6 adpcma channels + 1 adpcmb channel
		neoAudioIOWrite32(&SCHANNEL_SOURCE(i), (u32)NEOIPC->pAdpcmBuffer[i]); //adpcm buffer

		//NEO_ADPCMA_BUFFER_SIZE is size in samples, we get 2 samples per word
		neoAudioIOWrite32(&SCHANNEL_LENGTH(i), NEO_ADPCMA_BUFFER_SIZE / 2);

		neoAudioIOWrite16(&SCHANNEL_REPEAT_POINT(i), 0);
		neoAudioIOWrite16((vu16*)&SCHANNEL_TIMER(i), SOUND_FREQ(NEO_ADPCMA_RATE)); //sample rate
		
		neoAudioIOWrite32(&SCHANNEL_CR(i),
			SOUND_VOL(0x7f) |
			SOUND_PAN(64) |
			SOUND_REPEAT |
			SOUND_FORMAT_16BIT |
			SCHANNEL_ENABLE);
	}

	REG_IME = enabled;
}

static void neoAudioBufferWrite(volatile void* reg, u32 value, u32 bits)
{
	const s32 posProcessed = g_eventProcessed;
	s32 pos = g_eventSent;
	s32 pendingCount = pos - posProcessed;
	if(pendingCount < 0) pendingCount += NEO_EVENT_QUEUE_SIZE;

	if(pendingCount < NEO_EVENT_QUEUE_SIZE - 1) {
		TNeoAudioEvent* pEvent = &g_audioEvent[pos];
		pos++;
		if(pos >= NEO_EVENT_QUEUE_SIZE) {
			pos = 0;
		}
		pEvent->offset = (u32)reg - AUDIO_IO_BASE;
		pEvent->value = value;
		pEvent->bits = bits;
		neoAudioUpdate();
		pEvent->timeStamp = neoAudioGetTimestamp();
		pEvent->audioFrame = NEOIPC->audioStreamCount + 2;
		
		if(pendingCount == 0) {
			//start up timer (event will occur exactly 2 stream sizes in the future)
			TIMER_DATA(3) = 65536 - NEO_ADPCMA_STREAM_SIZE * 2;
			TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE | TIMER_IRQ_REQ;
		}
		
		g_eventSent = pos;
	} else {
		ASSERT(0);
		//can't add more events, so just do it now (better than nothing)
		TNeoAudioEvent e;
		e.bits = bits;
		e.value = value;
		e.offset = (u32)reg - AUDIO_IO_BASE;
		neoAudioDoWrite(&e);
	}
}

void neoAudioBufferWrite32(vu32* reg, u32 value, u32 mask)
{
	if((u32)reg < AUDIO_IO_BASE || (u32)reg >= AUDIO_IO_TOP) {
		return;
	}
	u32 oldValue = neoAudioIOMapRead32(reg);
	u32 newValue = (oldValue & ~mask) | (value & mask);
	if(newValue != oldValue) {
		neoAudioBufferWrite(reg, newValue, 32);
		neoAudioIOMapWrite32(reg, newValue);
	}
}

void neoAudioBufferWrite16(vu16* reg, u16 value, u16 mask)
{
	if((u32)reg < AUDIO_IO_BASE || (u32)reg >= AUDIO_IO_TOP) {
		return;
	}
	u16 oldValue = neoAudioIOMapRead16(reg);
	u16 newValue = (oldValue & ~mask) | (value & mask);
	if(newValue != oldValue) {
		neoAudioBufferWrite(reg, newValue, 16);
		neoAudioIOMapWrite16(reg, newValue);
	}
}

u32 neoAudioGetTimestamp()
{
	return TIMER_DATA(1) + NEO_ADPCMA_STREAM_SIZE - 65536;
}

void neoAudioPause()
{
	s32 i;
	//disable all SSG channels
	for(i = 11; i <= 15; i++) {
		SCHANNEL_CR(i) = 0;
	}
	SOUND_CR = SOUND_ENABLE;
}

void neoAudioResume()
{
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
}

void neoAudioUpdate()
{
	u32 timerData = TIMER_DATA(1);

	if(timerData < g_lastTimer) {
		u32 enabled = REG_IME;
		//must have wrapped
		NEOIPC->audioStreamCount++;
		//neoAudioStreamProcess();

		REG_IME = 0;
		const u32 audioFrame = NEOIPC->audioStreamCount;
		const s32 timeStamp = (s32)neoAudioGetTimestamp();
		//consume any events that may have been skipped over by irq
		while(g_eventSent != g_eventProcessed &&
			g_audioEvent[g_eventProcessed].audioFrame < audioFrame) {
				//process them all at once
				neoAudioDoWrite(&g_audioEvent[g_eventProcessed]);
				g_eventProcessed++;
				if(g_eventProcessed >= NEO_EVENT_QUEUE_SIZE) {
					g_eventProcessed = 0;
				}
		}
		
		if(!(TIMER_CR(3) & TIMER_ENABLE)) {
			//reset timer to proper value
			TIMER_CR(3) = 0;
			if(g_eventSent != g_eventProcessed) {
				s32 deltaT =
					(s32)g_audioEvent[g_eventProcessed].timeStamp - timeStamp;
				if(deltaT < 1) deltaT = 1;
				//still more events pending
				TIMER_DATA(3) = 65536 - deltaT;
				TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE | TIMER_IRQ_REQ;
			}
		}

		REG_IME = enabled;
	}
	g_lastTimer = timerData;
}
