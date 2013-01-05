#include "Default.h"
#include "EmuSystem.h"
#include "NeoSystem.h"
#include "NeoIPC.h"
#include "NeoProfiler.h"
#include "NeoAudioStream.h"

//pControl->frequency = (freq * 65536) / NEO_ADPCMA_RATE;
#define NEO_ADPCMB_MAXFREQ ((NEO_ADPCMB_RATE * 65536U) / NEO_ADPCMA_RATE)

static TNeoAdpcmControl g_adpcmControl[7][NEO_COMMAND_QUEUE_SIZE] ALIGN(32);

//buffers for each of the 7 channels
//6 adpcma, 1 adpcmb
static u16 g_adpcmaBuffer[7][NEO_ADPCMA_BUFFER_SIZE] ALIGN(32);
static u16* g_adpcmbBuffer;
static u16 g_audioFrame = 0;
static u32 g_buffer = 0;
static u32 g_zeroFill[7] = {0};

//static volatile u32 g_syncCount;
#define ADPCM_ADDR_MASK ((1 << 20) - 1)

#define YM_DELTAT_DELTA_MAX (24576)
#define YM_DELTAT_DELTA_MIN (127)
#define YM_DELTAT_DELTA_DEF (127)

static const s16 g_adpcmSteps[49] = {
	 16,  17,   19,   21,   23,   25,   28,
	 31,  34,   37,   41,   45,   50,   55,
	 60,  66,   73,   80,   88,   97,  107,
	118, 130,  143,  157,  173,  190,  209,
	230, 253,  279,  307,  337,  371,  408,
	449, 494,  544,  598,  658,  724,  796,
	876, 963, 1060, 1166, 1282, 1411, 1552
};

/* different from the usual ADPCM table */
static const s16 g_adpcmStepInc[16] = {
	-1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16,
	//BEN - duplicated first 8 values, avoid masking lower 3 bits when indexing table
	-1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16,
};

static const s8 g_adpcmbTable1[16] DTCM_DATA = {
	1,   3,   5,   7,   9,  11,  13,  15,
	-1,  -3,  -5,  -7,  -9, -11, -13, -15,
};

static const u8 g_adpcmbTable2[16] DTCM_DATA = {
	57,  57,  57,  57, 77, 102, 128, 153,
	57,  57,  57,  57, 77, 102, 128, 153
};

//speedup purposes only
//interleaved: acc value, step
s16 g_jediTable[49 * 16 * 2] ALIGN(32);

//this needs to fit in 8 bit index
//in addition, need 1 index for invalid, 1 for prev, and 1 for next
#define ADPCM_ENTRY_SHIFT 10
#define ADPCM_CACHE_COUNT 250
#define ADPCM_ROM_SIZE (16*MB)
#define ADPCM_ENTRY_SIZE (1 << ADPCM_ENTRY_SHIFT)
#define ADPCM_ENTRY_COUNT (ADPCM_ROM_SIZE / ADPCM_ENTRY_SIZE)

#if ADPCM_CACHE_COUNT < 253
#define ADPCM_INVALID_ENTRY 0xff
#else
#define ADPCM_INVALID_ENTRY 0xff00
#endif

typedef struct _TADPCMCacheEntry {
	u16 index;
#if ADPCM_CACHE_COUNT < 253
	u8 next;
	u8 prev;
#else
	u16 next;
	u16 prev;
#endif
} TADPCMCacheEntry;

#if ADPCM_CACHE_COUNT < 253
static u8 g_adpcmTable[ADPCM_ENTRY_COUNT] ALIGN(32);
#else
static u16 g_adpcmTable[ADPCM_ENTRY_COUNT] ALIGN(32);
#endif

#define ADPCM_CACHE_MAX (ADPCM_CACHE_COUNT + 2)
static u8 g_adpcmCache[ADPCM_CACHE_COUNT * ADPCM_ENTRY_SIZE] ALIGN(32);
static TADPCMCacheEntry g_adpcmEntry[ADPCM_CACHE_MAX] ALIGN(32);

#define LRU_HEAD ADPCM_CACHE_COUNT
#define LRU_TAIL (ADPCM_CACHE_COUNT+1)

void neoAudioStreamInit()
{
	s32 step, nib;
	u32 i;

	neoAudioStreamReset();

	g_adpcmbBuffer = (u16*)neoSystemVramHAlloc(NEO_ADPCMB_STREAM_SIZE * 2);

	g_neo->adpcmb.frequency = NEO_ADPCMA_RATE; //default to same rate as adpcma
	g_neo->adpcmb.freqCounter = 0;
	
	//build jedi table
	for (step = 0; step < 49; step++) {
		//loop over all nibbles and compute the difference
		for (nib = 0; nib < 16; nib++) {
			const s32 value = (2 * (nib & 0x07) + 1) * g_adpcmSteps[step] / 8;
			const s32 jediValue = (nib & 0x08) ? -value : value;
			s32 newStep = (step*16 + g_adpcmStepInc[nib]) * 4; //multiply by 4 because 4 bytes per jedi entry
			if(newStep > 48 * 16 * 4) newStep = 48 * 16 * 4;
			else if(newStep < 0) newStep = 0;
			//shift jedi table up by 4
			//(saves us from having to sign extend 12 bit values in decoder)
			g_jediTable[(step * 16 + nib) * 2] = jediValue << 4;
			g_jediTable[(step * 16 + nib) * 2 + 1] = newStep;
		}
	}

	neoSystemLoadRegion(NEOROM_AUDODATA, g_adpcmCache, ADPCM_CACHE_COUNT * ADPCM_ENTRY_SIZE);
	for(i = 0; i < ADPCM_ENTRY_COUNT; i++) {
		g_adpcmTable[i] = ADPCM_INVALID_ENTRY;
	}
	for(i = 0; i < ADPCM_CACHE_COUNT; i++) {
		g_adpcmEntry[i].index = i;
		g_adpcmEntry[i].next = i + 1;
		g_adpcmEntry[i].prev = i - 1;
		//g_adpcmEntry[i].frame = 0xffff;
		g_adpcmTable[i] = i;
	}
	g_adpcmEntry[0].prev = LRU_HEAD;
	g_adpcmEntry[ADPCM_CACHE_COUNT - 1].next = LRU_TAIL;

	g_adpcmEntry[LRU_HEAD].next = 0;
	g_adpcmEntry[LRU_HEAD].prev = ADPCM_INVALID_ENTRY;
	g_adpcmEntry[LRU_TAIL].next = ADPCM_INVALID_ENTRY;
	g_adpcmEntry[LRU_TAIL].prev = ADPCM_CACHE_COUNT - 1;

	g_neo->adpcmActive = 0;
}

void neoAudioStreamReset()
{
	u32 i;
	for(i = 0; i < 7; i++)  {
		NEOIPC->pAdpcmBuffer[i] = UNCACHED(g_adpcmaBuffer[i]);
		NEOIPC->adpcmControl[i] = UNCACHED(g_adpcmControl[i]);
		NEOIPC->adpcmQueuePos7[i] = 0;
		NEOIPC->adpcmQueuePos9[i] = 0;
		NEOIPC->adpcmaFinished[i] = 0;
		memset(g_adpcmaBuffer[i], 0, NEO_ADPCMA_BUFFER_SIZE);
		memset(g_adpcmControl[i], 0, sizeof(TNeoAdpcmControl) * NEO_COMMAND_QUEUE_SIZE);
		g_zeroFill[i] = 0;
	}
	DC_FlushAll();

	g_audioFrame = 0;
	g_buffer = 0;
}

/*void neoAudioAdpcmaDecode(u16* restrict pDst, const u8* restrict pSrc,
										   u32 length, TNeoADPCMStream* pAdpcm);

void neoAudioAdpcmaDecode(u16* restrict pDst, const u8* restrict pSrc,
										   u32 length, TNeoADPCMStream* pAdpcm)
{
	const u8* restrict pJedi = (u8*)g_jediTable;
	s32 acc = pAdpcm->acc;
	s32 step = pAdpcm->step;
	s32 jedi;
	s32 i;

	for(i = length; i > 0; i--) {
		const u32 data = *pSrc++;

		jedi = *(s32*)(pJedi + step + ((data & 0xf0) >> 2));
		acc += (s16)jedi;
		step = jedi >> 16;
		*pDst++ = acc;

		jedi = *(s32*)(pJedi + step + ((data & 0x0f) << 2));
		acc += (s16)jedi;
		step = jedi >> 16;
		*pDst++ = acc;
	}

	pAdpcm->acc = acc;
	pAdpcm->step = step;
}*/

static inline void markEntryAsUsed(const u32 cacheIndex)
{
	TADPCMCacheEntry* restrict pEntry = &g_adpcmEntry[cacheIndex];
	TADPCMCacheEntry* restrict pTail = &g_adpcmEntry[LRU_TAIL];
	ASSERT(cacheIndex < ADPCM_CACHE_MAX);

	//pEntry->frame = g_audioFrame;
	//unlink from list
	ASSERT(pEntry->next < ADPCM_CACHE_MAX);
	ASSERT(pEntry->prev < ADPCM_CACHE_MAX);
	g_adpcmEntry[pEntry->next].prev = pEntry->prev;
	g_adpcmEntry[pEntry->prev].next = pEntry->next;
	//hook to end of list
	ASSERT(pTail->prev < ADPCM_CACHE_MAX);
	pEntry->prev = pTail->prev;
	pEntry->next = LRU_TAIL;
	g_adpcmEntry[pTail->prev].next = cacheIndex;
	pTail->prev = cacheIndex;
}

static void* neoAudioStream(const u32 addr, s32* length)
{
	const u32 entryIndex = addr >> ADPCM_ENTRY_SHIFT;
	//how many bytes into the current entry to read
	const u32 entryOffset = addr & (ADPCM_ENTRY_SIZE - 1);

	ASSERT(length != 0);
	ASSERT(*length > 0);
	ASSERT(addr + *length < neoSystemRegionSize(NEOROM_AUDODATA));
	ASSERT(entryIndex < ADPCM_ENTRY_COUNT);

	u32 entry = g_adpcmTable[entryIndex];
	if(entry == ADPCM_INVALID_ENTRY) {
		//replace LRU cache entry
		entry = g_adpcmEntry[LRU_HEAD].next;
		ASSERT(entry < ADPCM_CACHE_COUNT);
		//mark previous entry as invalid
		ASSERT(g_adpcmEntry[entry].index < ADPCM_ENTRY_COUNT);
		g_adpcmTable[g_adpcmEntry[entry].index] = ADPCM_INVALID_ENTRY;
		//connect currenty entry
		g_adpcmTable[entryIndex] = entry;
		g_adpcmEntry[entry].index = entryIndex;
		//read data into cache
		neoSystemReadRegion(NEOROM_AUDODATA,
			&g_adpcmCache[entry << ADPCM_ENTRY_SHIFT], //dst
			entryIndex * ADPCM_ENTRY_SIZE, //rom offset
			ADPCM_ENTRY_SIZE); //size to read
	}
	ASSERT(entry < ADPCM_CACHE_COUNT);

	//flag entry as recently used
	markEntryAsUsed(entry);

	const s32 transferSize = *length;
	if(transferSize > ADPCM_ENTRY_SIZE - entryOffset) {
		*length = ADPCM_ENTRY_SIZE - entryOffset;
	}
	return &g_adpcmCache[(entry << ADPCM_ENTRY_SHIFT) + entryOffset];
}

static void neoAudioStreamAdpcma(u32 ch, u16* restrict pBuffer, s32 baseStreamSize)
{
	TNeoADPCMStream* restrict pAdpcm = &g_neo->adpcm[ch];
	u16* restrict pDst = pBuffer;
	s32 i;

	ASSERT(ch < 6);
	ASSERT(baseStreamSize >= 0);
	if(baseStreamSize == 0) {
		return;
	}

	profilerPush(NEOPROFILE_AUDIO_ADPCMA);

	if((g_neo->adpcmActive & (1 << ch))) {
		const u32 end = pAdpcm->end & ADPCM_ADDR_MASK;
		const u8* restrict pJediTable = (u8*)g_jediTable;
		s32 acc = pAdpcm->acc;
		u32 offset = pAdpcm->offset;
		const u8* restrict pJedi = pJediTable + pAdpcm->step;

		while(baseStreamSize > 0) {
			s32 streamSize = baseStreamSize;
			
			if((offset & ADPCM_ADDR_MASK) <= end && (offset & ADPCM_ADDR_MASK) + streamSize >= end) {
				streamSize = end - (offset & ADPCM_ADDR_MASK);
			}
			
			ASSERTMSG(streamSize >= 0 && streamSize <= baseStreamSize,
				"streamSize: %d, baseStreamSize: %d", streamSize, baseStreamSize);
			if(streamSize > 0) {
				const u8* restrict pSrc = neoAudioStream(offset, &streamSize);
				s32 jedi;

				ASSERTMSG(streamSize > 0 && streamSize <= baseStreamSize,
					"streamSize: %d, baseStreamSize: %d", streamSize, baseStreamSize);
				offset += streamSize;
				baseStreamSize -= streamSize;

				for(i = streamSize; i > 0; i--) {
					const u32 data = *pSrc++;

					//read 2 consecutive 16 bit values from lookup table
					jedi = *(s32*)(pJedi + ((data & 0xf0) >> 2));
					acc += (s16)jedi; //lower value is acc inc
					pJedi = pJediTable + (jedi >> 16); //upper value is new step
					*pDst++ = acc;

					jedi = *(s32*)(pJedi + ((data & 0x0f) << 2));
					acc += (s16)jedi;
					pJedi = pJediTable + (jedi >> 16);
					*pDst++ = acc;
				}
			}

			if((offset & ADPCM_ADDR_MASK) == end) {
				g_neo->adpcmActive &= ~(1 << ch);
				NEOIPC->adpcmaFinished[ch] = 1;
				break;
			}
			ASSERT(streamSize > 0);
		}
		ASSERT(baseStreamSize >= 0);
		g_zeroFill[ch] = baseStreamSize * 2;
		for(i = baseStreamSize; i > 0; i--) {
			*pDst++ = 0;
			*pDst++ = 0;
		}
		pAdpcm->acc = acc;
		pAdpcm->step = (s32)(pJedi - pJediTable);
		pAdpcm->offset = offset;
	} else if(g_zeroFill[ch] < NEO_ADPCMA_BUFFER_SIZE) {
		g_zeroFill[ch] += baseStreamSize * 2;
		for(i = baseStreamSize; i > 0; i--) {
			*pDst++ = 0;
			*pDst++ = 0;
		}
	}

	profilerPop();
}

static void neoAudioStreamAdpcmb(u16* restrict pBuffer, const s32 baseStreamSize)
{
	TNeoADPCMStream* restrict pAdpcm = &g_neo->adpcm[6];
	s32 i;

	ASSERT(baseStreamSize >= 0);
	if(baseStreamSize == 0) {
		return;
	}

	profilerPush(NEOPROFILE_AUDIO_ADPCMB);

	if((g_neo->adpcmActive & (1 << 6))) {
		const u32 end = pAdpcm->end & ADPCM_ADDR_MASK;
		const u32 loop = (g_neo->adpcmb.initOffset >> 31) & 0x01;
		const s32 audio2Offset = g_header.audio2Offset;
		//keep these values in locals, make sure compiler knows we want them in regs
		const s32 initAcc = pAdpcm->acc;
		s32 acc = initAcc;
		s32 step = pAdpcm->step;
		s32 offset = pAdpcm->offset;
		const s32 freq = g_neo->adpcmb.frequency;
		s32 freqCounter = g_neo->adpcmb.freqCounter;
		//how much data do we need to read, to stream the proper ammount at the current sample rate?
		const s32 totalBytesRead =
			((u32)((u32)freqCounter + (u32)freq * (u32)baseStreamSize) >> 16) + 1;
		s32 bytesToRead = totalBytesRead;
		u16* restrict pStreamDst = g_adpcmbBuffer;
		
		ASSERTMSG(freqCounter >= 0 && freqCounter <= 0xffff, "freqCounter: %d", freqCounter);
		ASSERTMSG(freq >= 0 && freq <= NEO_ADPCMB_MAXFREQ, "freq: %d", freq);
		ASSERTMSG(bytesToRead > 0 && bytesToRead <= NEO_ADPCMB_STREAM_SIZE,
			"bytesToRead: %d, baseStreamSize: %d, freq: %d",
			bytesToRead, baseStreamSize, freq);
		
		while(bytesToRead > 0) {
			s32 bytesRead = bytesToRead;

			if((offset & ADPCM_ADDR_MASK) <= end && (offset & ADPCM_ADDR_MASK) + bytesRead >= end) {
				bytesRead = end - (offset & ADPCM_ADDR_MASK);
			}
			
			ASSERT(bytesRead <= bytesToRead);
			ASSERT(bytesRead >= 0);
			if(bytesRead > 0) {
				const u8* restrict pSrc = neoAudioStream(audio2Offset + offset, &bytesRead);
				ASSERT(bytesRead <= bytesToRead);
				ASSERT(bytesRead > 0);

				//decode adpcm data into temporary buffer
				for(i = bytesRead; i > 0; i--) {
					const u32 data = *pSrc++;

					const u32 d0 = data >> 4;
					acc += (g_adpcmbTable1[d0] * step / 8);
					step = (step * g_adpcmbTable2[d0]) / 64;
					if(acc > 0x7fff) acc = 0x7fff;
					else if(acc < -0x8000) acc = -0x8000;
					if(step > YM_DELTAT_DELTA_MAX) step = YM_DELTAT_DELTA_MAX;
					else if(step < YM_DELTAT_DELTA_MIN) step = YM_DELTAT_DELTA_MIN;
					*pStreamDst++ = acc;

					const u32 d1 = data & 0x0f;
					acc += (g_adpcmbTable1[d1] * step / 8);
					step = (step * g_adpcmbTable2[d1]) / 64;
					if(acc > 0x7fff) acc = 0x7fff;
					else if(acc < -0x8000) acc = -0x8000;
					if(step > YM_DELTAT_DELTA_MAX) step = YM_DELTAT_DELTA_MAX;
					else if(step < YM_DELTAT_DELTA_MIN) step = YM_DELTAT_DELTA_MIN;
					*pStreamDst++ = acc;
				}

				offset += bytesRead;
				bytesToRead -= bytesRead;
			}

			if((offset & ADPCM_ADDR_MASK) == end) {
				//if we still have more to go, must have hit end of sample
				if(loop) {
					//keep looping back at start of sample
					offset = g_neo->adpcmb.initOffset & 0x7fffffff;
					acc = 0;
					step = YM_DELTAT_DELTA_DEF;
					systemWriteLine("adpcmb looped");
				} else {
					//all done, break loop
					g_neo->adpcmActive &= ~(1 << 6);
					NEOIPC->adpcmaFinished[6] = 1;
					//systemWriteLine("adpcmb done (no loop)");
					break;
				}
			} else {
				ASSERTMSG(bytesRead > 0, "neoAudioStreamAdpcmb: stuck");
			}
		}

		ASSERT(bytesToRead >= 0);
		for(i = 0; i < bytesToRead; i++) {
			*pStreamDst++ = 0;
			*pStreamDst++ = 0;
		}

		//copy decoded data to stream buffer, interpolated for proper sample rate
		const u32 mask = 0xffff; //want this value in register
		const u16* restrict pSrc = g_adpcmbBuffer;
		u16* restrict pDst = pBuffer;
		acc = initAcc;
		for(i = baseStreamSize; i > 0; i--) {
			freqCounter += freq;
			if(freqCounter >= 0x10000) {
				ASSERTMSG(pSrc < &g_adpcmbBuffer[totalBytesRead * 2], "overrun: %d (%d / %d, f:%d)",
					(int)pSrc - (int)&g_adpcmbBuffer[totalBytesRead * 2], totalBytesRead, baseStreamSize, freq);
				acc = *pSrc;
				pSrc += (freqCounter >> 16);
				freqCounter &= mask;
			}
			*pDst++ = acc;
			freqCounter += freq;
			if(freqCounter >= 0x10000) {
				ASSERTMSG(pSrc < &g_adpcmbBuffer[totalBytesRead * 2], "overrun: %d (%d / %d, f:%d)",
					(int)pSrc - (int)&g_adpcmbBuffer[totalBytesRead * 2], totalBytesRead, baseStreamSize, freq);
				acc = *pSrc;
				pSrc += (freqCounter >> 16);
				freqCounter &= mask;
			}
			*pDst++ = acc;
		}

		//write back to data struct
		pAdpcm->acc = acc;
		pAdpcm->step = step;
		pAdpcm->offset = offset;
		g_neo->adpcmb.freqCounter = freqCounter;
	} else if(g_zeroFill[6] < NEO_ADPCMA_BUFFER_SIZE) {
		u16* restrict pDst = pBuffer;
		g_zeroFill[6] += baseStreamSize * 2;
		for(i = baseStreamSize; i > 0; i--) {
			*pDst++ = 0;
			*pDst++ = 0;
		}
	}

	profilerPop();
}

static inline void neoAudioStreamAdpcm(u32 ch, u16* restrict pBuffer, u32 baseStreamSize)
{
	ASSERT(ch < 7);
	if(ch < 6) neoAudioStreamAdpcma(ch, pBuffer, baseStreamSize);
	else neoAudioStreamAdpcmb(pBuffer, baseStreamSize);
}

static void neoAudioCommand(u32 ch, const TNeoAdpcmControl* restrict pControl)
{
	ASSERT(ch < 7);
	TNeoADPCMStream* restrict pAdpcm = &g_neo->adpcm[ch];

	switch(pControl->command) {
	case NEOADPCM_START:
		pAdpcm->offset = pControl->startAddr & 0x7fffffff;
		pAdpcm->acc = 0;
		if(ch < 6) {
			//adpcm a
			pAdpcm->step = 0;
		} else {
			//different default step value for adpcmb
			pAdpcm->step = YM_DELTAT_DELTA_DEF;
			//addition settings for adpcmb
			g_neo->adpcmb.initOffset = pControl->startAddr;
			g_neo->adpcmb.freqCounter = 0;
		}
		if(pAdpcm->offset < NEOIPC->audioRomSize) {
			//extra safety check for address bounds
			g_neo->adpcmActive |= (1 << ch);
			g_zeroFill[ch] = 0;
		}
		break;
	case NEOADPCM_ENDADDR:
		pAdpcm->end = pControl->endAddr;
		break;
	case NEOADPCM_STOP:
		g_neo->adpcmActive &= ~(1 << ch);
		break;
	case NEOADPCM_FREQUENCY:
		ASSERTMSG(ch == 6, "NEOADPCM_FREQUENCY on adpcma (%d)", ch);
		if(ch == 6) {
			//only valid for adpcmb
			ASSERT(pControl->frequency <= NEO_ADPCMB_MAXFREQ);
			g_neo->adpcmb.frequency = pControl->frequency;
		}
		break;
	}
}

static s32 neoAudioSkipCommands(u32 ch, s32 adpcmQueuePos7, s32 adpcmQueuePos9)
{
	s32 commandIndex = adpcmQueuePos9;
	s32 commandCount = adpcmQueuePos7 - commandIndex;
	u32 i;
	
	if(commandCount < 0) commandCount += NEO_COMMAND_QUEUE_SIZE;
	
	ASSERT(commandCount < NEO_COMMAND_QUEUE_SIZE);
	ASSERT(commandIndex < NEO_COMMAND_QUEUE_SIZE);

#ifndef NEO_SHIPPING
	if(commandCount > 0) {
		systemWriteLine("skip %d commands on ch %d", commandCount, ch);
	}
#endif

	for(i = 0; i < commandCount; i++) {
		const TNeoAdpcmControl* restrict pControl = &NEOIPC->adpcmControl[ch][commandIndex];
		if(pControl->audioFrame != g_audioFrame) {
			systemWriteLine("process break");
			break;
		}

		neoAudioCommand(ch, pControl);

		commandIndex++;
		if(commandIndex >= NEO_COMMAND_QUEUE_SIZE) {
			commandIndex = 0;
		}
	}
	//update arm9 queue pos to arm7 queue pos
	return commandIndex;
}

static s32 neoAudioProcessChannel(u32 ch, s32 adpcmQueuePos7, s32 adpcmQueuePos9)
{
	//TNeoADPCMStream* restrict pAdpcm = &g_neo->adpcm[ch];
	u32 sizeToTransfer = NEO_ADPCMA_STREAM_SIZE / 2;
	s32 commandIndex = adpcmQueuePos9;
	s32 commandCount = adpcmQueuePos7 - commandIndex;
	u32 lastCommand = 0;
	u32 bufferPos = g_buffer;
	u32 i;
	if(commandCount < 0) commandCount += NEO_COMMAND_QUEUE_SIZE;

	ASSERT(commandCount < NEO_COMMAND_QUEUE_SIZE);
	ASSERT(commandIndex < NEO_COMMAND_QUEUE_SIZE);

/*#ifndef NEO_SHIPPING
	if(commandCount > 0) {
		systemWriteLine("process %d commands on ch %d", commandCount, ch);
	}
#endif*/

	for(i = 0; i < commandCount; i++) {
		const TNeoAdpcmControl* restrict pControl = &NEOIPC->adpcmControl[ch][commandIndex];
		if(pControl->audioFrame != g_audioFrame) {
			//this command will wait for the next frame (we're behind in this case)
			ASSERTMSG((u16)(g_audioFrame + 1) == pControl->audioFrame,
				"index: %d, frame: %d / %d, cmd: %d / %d",
				commandIndex,
				g_audioFrame, pControl->audioFrame,
				i, commandCount);
			break;
		}
		//process up until command
		//currently process 2 samples per iteration, hence the divide by 2
		u32 transferSize = (pControl->timeStamp - lastCommand) / 2;
		//we process from the last command until the current command
		lastCommand = pControl->timeStamp;
		
		if(transferSize > 0) {
			//ASSERT(sizeToTransfer >= transferSize);
			if(transferSize > sizeToTransfer) {
				//i think the divide by 2's occational make this off by 1
				transferSize = sizeToTransfer;
			}
			ASSERTMSG(bufferPos + transferSize * 2 <= NEO_ADPCMA_BUFFER_SIZE,
				"overflow %d + %d * 2", bufferPos, transferSize);
			neoAudioStreamAdpcm(ch, &g_adpcmaBuffer[ch][bufferPos], transferSize);
			sizeToTransfer -= transferSize;
			bufferPos += transferSize * 2; //2 samples per transfered byte
		}
		
		neoAudioCommand(ch, pControl);

		commandIndex++;
		if(commandIndex >= NEO_COMMAND_QUEUE_SIZE) {
			commandIndex = 0;
		}
	}
	
	//process the remaining
	if(sizeToTransfer > 0) {
		ASSERTMSG(bufferPos + sizeToTransfer * 2 <= NEO_ADPCMA_BUFFER_SIZE,
			"overflow %d + %d * 2", bufferPos, sizeToTransfer);
		neoAudioStreamAdpcm(ch, &g_adpcmaBuffer[ch][bufferPos], sizeToTransfer);
	}

	//update arm9 queue pos to arm7 queue pos
	return commandIndex;
}

/*void neoAudioSetEnabled(bool enable)
{
	s32 i;

	if(NEOIPC->globalAudioEnabled) {
		if(enable == NEOIPC->audioEnabled) return;
		NEOIPC->audioEnabled = enable;
		if(!enable) {
			g_neo->adpcmActive = 0;
		} else {
			for(i = 0; i < 7; i++) {
				NEOIPC->adpcmaFinished[i] = 1;
			}
		}
	} else {
		NEOIPC->audioEnabled = false;
	}
}*/

void neoAudioStreamProcess()
{
	s16 adpcmQueuePos7[7];
	s16 adpcmQueuePos9[7];
	u32 ch;

	if(!NEOIPC->globalAudioEnabled) {
		return;
	}

	for(ch = 0; ch < 7; ch++) {
		adpcmQueuePos7[ch] = NEOIPC->adpcmQueuePos7[ch];
		adpcmQueuePos9[ch] = NEOIPC->adpcmQueuePos9[ch];
		ASSERT(adpcmQueuePos7[ch] >= 0);
		ASSERT(adpcmQueuePos7[ch] < NEO_COMMAND_QUEUE_SIZE);
		ASSERT(adpcmQueuePos9[ch] >= 0);
		ASSERT(adpcmQueuePos9[ch] < NEO_COMMAND_QUEUE_SIZE);
	}
	const u16 arm7StreamCount = NEOIPC->audioStreamCount;

	if(g_audioFrame != arm7StreamCount) {
		profilerPush(NEOPROFILE_AUDIO);

		//gracefully handle skips
		while((u16)(g_audioFrame + 1) != (u16)arm7StreamCount) {
			for(ch = 0; ch < 7; ch++) {
				adpcmQueuePos9[ch] =
					neoAudioSkipCommands(ch, adpcmQueuePos7[ch], adpcmQueuePos9[ch]);
			}
			if(g_buffer == 0) g_buffer = NEO_ADPCMA_STREAM_SIZE;
			else g_buffer = 0;
			g_audioFrame++;
			systemWriteLine("adpcma skip (%d/%d)", arm7StreamCount);
		}
		//process channels
		for(ch = 0; ch < 7; ch++) {
			adpcmQueuePos9[ch] =
				neoAudioProcessChannel(ch, adpcmQueuePos7[ch], adpcmQueuePos9[ch]);
		}
		//advance frame
		if(g_buffer == 0) g_buffer = NEO_ADPCMA_STREAM_SIZE;
		else g_buffer = 0;
		g_audioFrame++;

		//update arm9 position
		for(ch = 0; ch < 7; ch++) {
			NEOIPC->adpcmQueuePos9[ch] = adpcmQueuePos9[ch];
		}

		profilerPop();
	}
}
