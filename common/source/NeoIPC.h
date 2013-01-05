#ifndef _NEO_IPC_H
#define _NEO_IPC_H

#define NEOIPC_MAX_ARGS 4

#define NEO_COMMAND_QUEUE_SIZE 32

#define NEOIPC_ADDR 0x027FF400

//#define NEOIPC_Z80DAATABLE NEOIPC_ADDR
#define NEOIPC_AUDIOPROGRAM0 (NEOIPC_ADDR)
#define NEOIPC_AUDIOPROGRAMSIZE (NEOIPC_AUDIOPROGRAM0+4)
#define NEOIPC_AUDIOROMSIZE (NEOIPC_AUDIOPROGRAMSIZE+4)

#define NEOIPC_ARM9FIFOSENT (NEOIPC_AUDIOROMSIZE+4)
#define NEOIPC_ARM9FIFOPROCESSED (NEOIPC_ARM9FIFOSENT+4)
#define NEOIPC_ARM7FIFOSENT (NEOIPC_ARM9FIFOPROCESSED+4)
#define NEOIPC_ARM7FIFOPROCESSED (NEOIPC_ARM7FIFOSENT+4)

#define NEOIPC_ADPCMABUFFER (NEOIPC_ARM7FIFOPROCESSED+4)
#define NEOIPC_ADPCMACONTROL (NEOIPC_ADPCMABUFFER + 7 * 4)
#define NEOIPC_ADPCMAQUEUEPOS7 (NEOIPC_ADPCMACONTROL + 7 * 4)
#define NEOIPC_ADPCMAQUEUEPOS9 (NEOIPC_ADPCMAQUEUEPOS7 + 7 * 2)
//#define NEOIPC_ARM9STATE (NEOIPC_ADPCMAQUEUEPOS9 + 7 * 4)
//#define NEOIPC_ARM7STATE (NEOIPC_ARM9STATE+4)
#define NEOIPC_ARM7ALIVE (NEOIPC_ADPCMAQUEUEPOS9 + 7 * 2)

#define NEOIPC_ARM9ARGS (NEOIPC_ARM7ALIVE+4)
#define NEOIPC_ARM9RETURN (NEOIPC_ARM9ARGS + 4 * NEOIPC_MAX_ARGS)

#define NEOIPC_AUDIOSTREAMCOUNT (NEOIPC_ARM9RETURN+4)

#define NEOIPC_ADPCMAFINISHED (NEOIPC_AUDIOSTREAMCOUNT+2)
//#define NEOIPC_ARM9COMMAND (NEOIPC_ADPCMAFINISHED+6)
//#define NEOIPC_ARM7COMMAND (NEOIPC_ARM9COMMAND+1)
//#define NEOIPC_AUDIOFORCENMI (NEOIPC_ARM7COMMAND+1)
//#define NEOIPC_AUDIOENABLED (NEOIPC_ADPCMAFINISHED+7)
#define NEOIPC_GLOBALAUDIOENABLED (NEOIPC_ADPCMAFINISHED+7)
#define NEOIPC_AUDIOCOMMAND (NEOIPC_GLOBALAUDIOENABLED+1)
#define NEOIPC_AUDIORESULT (NEOIPC_AUDIOCOMMAND+1)
#define NEOIPC_AUDIOCOMMANDPENDING (NEOIPC_AUDIORESULT+1)
#define NEOIPC_MISC (NEOIPC_AUDIOCOMMANDPENDING+1)
#define NEOIPC_END (NEOIPC_MISC+1)

#ifndef NEO_IN_ASM

//commands sent by arm7 to arm9 via IPC_SendSync
//enum _TNeoArm9SyncCommand {
//	NEOSYNC9_UPDATESTREAM,
//} TNeoArm9SyncCommand;

//typedef enum _TNeoArm9Command {
//	NEOARM9_NONE,
//} TNeoArm9Command;

//typedef enum _TNeoArm7Command {
//	NEOARM7_NONE,
//	NEOARM7_RESET,
//} TNeoArm7Command;

typedef enum _TNeoAdpcmCommand {
	NEOADPCM_NONE,
	NEOADPCM_STOP,
	NEOADPCM_START,
	NEOADPCM_ENDADDR,
	NEOADPCM_FREQUENCY,
} TNeoAdpcmCommand;

typedef struct _TNeoAdpcaControl {
	u32 command; //TNeoAdpcmaCommand
	union {
		u32 startAddr;
		u32 endAddr;
		u32 frequency;
	};
	u16 audioFrame; //which audio frame does this command occur in?
	u16 timeStamp; //how many samples into the audio frame did this command occur?
} TNeoAdpcmControl;

typedef struct _TNeoIPC {
	//const u16* pZ80DaaTable;
	u8* pAudioProgram0;
	u32 audioProgramSize;
	u32 audioRomSize;
	u32 arm9FifoSent;
	u32 arm9FifoProcessed;
	u32 arm7FifoSent;
	u32 arm7FifoProcessed;
	u16* pAdpcmBuffer[7];
	TNeoAdpcmControl* adpcmControl[7]; //commands from arm7 to arm9
	s16 adpcmQueuePos7[7];
	s16 adpcmQueuePos9[7];
	u32 arm7Alive; //arm7 incs this every frame, helps arm9 know if arm7 crashed

	//for calling functions on other system
	u32 arm9Args[NEOIPC_MAX_ARGS];
	u32 arm9Return;

	u16 audioStreamCount; //inc by arm7 when time for audio to stream, dec by arm9 when audio actually streams
	
	u8 adpcmaFinished[7]; //set by arm9 when channel finished, read and cleared by arm7
	
	//u8 audioEnabled;
	u8 globalAudioEnabled;
	u8 audioCommand;
	u8 audioResult;
	u8 audioCommandPending;
	u8 misc; //used for random debugging things
} TNeoIPC;

//regular IPC occurs at 0x027FF000, ld script reserves 4*KB,
//so we start ours 2*KB after
#define NEOIPC ((volatile TNeoIPC*)NEOIPC_ADDR)

typedef enum _TNeoIPCCommand {
	NEOARM7_RESET = 1,
	NEOARM7_PAUSE,
	NEOARM7_RESUME,
	NEOARM7_NMI,
	NEOARM7_BACKLIGHTOFF,
	NEOARM7_BACKLIGHTON,
	NEOARM7_LIDCLOSE,
	NEOARM7_LIDOPEN,
	NEOARM9_READAUDIO,
	NEOARM9_AUDIORESULT,
} TNeoIPCCommand;

#define NEOIPC_COMMAND_SHIFT 24
#define NEOIPC_COMMAND_MASK 0xff000000
#define NEOIPC_ARG_SHIFT 0
#define NEOIPC_ARG_MASK 0x00ffffff

#define NEOIPC_GET_COMMAND(data) \
	((data & NEOIPC_COMMAND_MASK) >> NEOIPC_COMMAND_SHIFT)

#define NEOIPC_GET_ARG(data) \
	((data & NEOIPC_ARG_SHIFT) >> NEOIPC_ARG_MASK)

void neoIPCInit();
u32 neoIPCSendCommandAsync(TNeoIPCCommand command);
void neoIPCSendCommand(TNeoIPCCommand command);
bool neoIPCCheckCommandDone(u32 message);
void neoIPCWaitCommandDone(u32 message);
u32 neoIPCRecvCommand();
u32 neoIPCWaitCommand(TNeoIPCCommand command);
void neoIPCAckCommand();

#endif //NEO_IN_ASM

#endif
