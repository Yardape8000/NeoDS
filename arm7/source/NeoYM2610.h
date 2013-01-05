#ifndef _NEO_YM2610_H
#define _NEO_YM2610_H

typedef struct _TADPCMAChannel {
	u32 start; //sample data start address (in bytes)
	u32 end; //sample data end address (in bytes)
	u8 level; //volume
	u8 pan; //bit 0: right enable, bit 1: left enable
	u8 flagMask; //or-ed with adpcmArriveEnd when channel finished playing
} TADPCMAChannel;

typedef struct _TADPCMBChannel {
	u32 start;
	u32 end;
	u32 delta;
	u32 lastDelta; //last delta that was sent to the arm9
	u8 level; //volume
	u8 portstate;
	u8 control;
	u8 pan; //bit 0: left enable, bit 1: right enable (opposite of adpcma)
	u8 flagMask;
} TADPCMBChannel;

typedef enum _TSSGChannelState {
	SSG_CHANNEL_DISABLE,
	SSG_CHANNEL_TONE,
	SSG_CHANNEL_NOISE,
	SSG_CHANNEL_NOISETONE, //both enabled
	SSG_CHANNEL_WAIT_NOISE,
	SSG_CHANNEL_WAIT_NOISETONE,
} TSSGChannelState;

typedef struct _TSSGChannel {
	u32 period;
	u32 volume;
	u8 envelopeEnable;
	u8 hwChannel; //chich hw channel is this mapped to?
	u8 state;
} TSSGChannel;

typedef struct _TSSGNoise {
	u32 period;
	//s32 count;
	//s32 rng;
	//u8 output;
} TSSGNoise;

typedef struct _TSSGEnvelope {
	u32 period;
	s32 count;
	u32 volume;
	s32 index;
	u8 attack;
	u8 alternate;
	u8 hold;
	u8 holding;
} TSSGEnvelope;

typedef struct _TYM2610Context {
	//structs for all different channels
	TADPCMAChannel adpcma[6];
	TADPCMBChannel adpcmb;
	TSSGChannel ssg[3];

	TSSGNoise noise;
	TSSGEnvelope ssgEnvelope;
	
	s32 timerA;
	s32 timerB;
	s32 timerAValue;
	s32 timerBValue;
	s32 timerATicks; //how many updates till timer fires
	s32 timerBTicks; //how many updates till timer fires
	u32 irqStatus;
	s32 queuePos[7];

	u8 reg[512];
	//named registers
	u8 addrPort;
	u8 address;
	u8 mode;

	u8 adpcmaTotalLevel;
	u8 adpcmArriveEnd;
	u8 irq;
	u8 noiseChannelUsed; //2 bit mask for 2 noise channels (14 and 15)
} TYM2610Context;

//switch(a)
//case 0: control A
//case 1: data A
//case 2: control B
//case 3: data B
void neoYM2610Write(u16 a, u8 d);

//switch(a)
//case 0: status A
//case 1: data
//case 2: status B
u8 neoYM2610Read(u16 a);

void neoYM2610Init();
void neoYM2610Process();

#endif
