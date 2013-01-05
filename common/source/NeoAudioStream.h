#ifndef _NEO_AUDIO_STREAM_H
#define _NEO_AUDIO_STREAM_H

//sample rate (Hz) of adpcma audio channel
#define NEO_ADPCMA_RATE 18432 //18500 in reality, but fudged a bit to make numbers nicer...see below)

//max sample rate (Hz) of adpcmb audio channel
#define NEO_ADPCMB_RATE 55500

//how many times per second is neoAudioStreamUpdate processed
#define NEO_STREAM_RATE 18

//NEO_ADPCMA_RATE and NEO_STREAM_RATE set such that NEO_ADPCMA_STREAM_SIZE is 1024 or 2048
//adjust ADPCM cache so that 1 cache entry == NEO_ADPCMA_STREAM_SIZE
#define NEO_ADPCMA_STREAM_SIZE (NEO_ADPCMA_RATE / NEO_STREAM_RATE)
#define NEO_ADPCMA_BUFFER_SIZE (NEO_ADPCMA_STREAM_SIZE * 2)

#define NEO_ADPCMB_STREAM_SIZE RoundUp4B(NEO_ADPCMB_RATE / NEO_STREAM_RATE + 4)
//#define NEO_ADPCMB_BUFFER_SIZE (NEO_ADPCMB_STREAM_SIZE * 2)

#ifdef ARM9

typedef struct _TNeoADPCMStream {
	u32 offset;
	u32 end;
	s16 acc;
	s16 step;
} TNeoADPCMStream;

//additional values need for ADPCMB channel
typedef struct _TNeoADPCMBStream {
	u32 initOffset;
	s32 freqCounter;
	s32 frequency;
} TNeoADPCMBStream;

void neoAudioStreamInit();
void neoAudioStreamReset();
void neoAudioStreamProcess();
//void neoAudioSetEnabled(bool enable);

#endif

#endif
