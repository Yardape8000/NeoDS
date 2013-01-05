#include "nds.h"
#include "NeoCpuZ80.h"
#include "NeoIPC.h"
#include "NeoYM2610.h"
#include "NeoAudio.h"
#include "NeoSystem7.h"

STATIC_ASSERT(sizeof(TNeoContext7) == NEO_CONTEXTEND7);

static TNeoContext7 g_neoContext7;

static void neoBacklightOff()
{
	u32 enabled = REG_IME;
	REG_IME = 0;
	u32 value = readPowerManagement(PM_CONTROL_REG);
	writePowerManagement(PM_CONTROL_REG, value & ~PM_BACKLIGHT_BOTTOM);
	REG_IME = enabled;
}

static void neoBacklightOn()
{
	u32 enabled = REG_IME;
	REG_IME = 0;
	u32 value = readPowerManagement(PM_CONTROL_REG);
	writePowerManagement(PM_CONTROL_REG, value | PM_BACKLIGHT_BOTTOM);
	REG_IME = enabled;
}

static void neoLidClose()
{
	u32 enabled = REG_IME;
	REG_IME = 0;
	const u32 powerValue = readPowerManagement(PM_CONTROL_REG);
	u32 value = powerValue;
	value &= ~(PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP);
	value |= PM_LED_SLEEP;
	writePowerManagement(PM_CONTROL_REG, value);
	REG_IME = enabled;
	swiChangeSoundBias(0, 0x400); //speaker off
	neoAudioShutdown();

	//ack LIDCLOSE command
	neoIPCAckCommand();

	//wait for lid to open
	while(1) {
		swiWaitForVBlank();
		u32 command = neoIPCRecvCommand();
		if(command) {
			if(NEOIPC_GET_COMMAND(command) == NEOARM7_LIDOPEN) {
				break;
			} else {
				//ignore all commands (should not get any)
				neoIPCAckCommand();
			}
		}
	}

	enabled = REG_IME;
	REG_IME = 0;
	writePowerManagement(PM_CONTROL_REG, powerValue);
	REG_IME = enabled;
	swiChangeSoundBias(1, 0x400); //speaker on
	neoAudioReset();

	//ACK lid open command
	neoIPCAckCommand();
}

static void neoSystem7Pause()
{
	bool paused = true;

	neoAudioPause();

	//acknowledge pause command
	neoIPCAckCommand();

	while(paused) {
		u32 command = neoIPCRecvCommand();
		if(command != 0) {
			switch(NEOIPC_GET_COMMAND(command)) {
			case NEOARM7_RESET:
				neoSystem7Reset();
				paused = false;
				neoIPCAckCommand();
				break;
			//resume only works when paused
			case NEOARM7_RESUME:
				paused = false;
				neoIPCAckCommand();
				break;
			case NEOARM7_BACKLIGHTOFF:
				neoBacklightOff();
				neoIPCAckCommand();
				break;
			case NEOARM7_BACKLIGHTON:
				neoBacklightOn();
				neoIPCAckCommand();
				break;
			//lid close only works when paused
			case NEOARM7_LIDCLOSE:
				neoLidClose();
				break;
			default:
				//ignore
				neoIPCAckCommand();
				break;
			}
		}
		if(NEOIPC->globalAudioEnabled) {
			neoAudioUpdate();
		}
		swiWaitForVBlank();
	}

	neoAudioResume();
}

static void neoSystem7ProcessCommands()
{
	u32 command = neoIPCRecvCommand();
	while(command != 0) {
		switch(NEOIPC_GET_COMMAND(command)) {
		case NEOARM7_NMI:
			neoZ80Nmi();
			neoIPCAckCommand();
			break;
		case NEOARM7_RESET:
			neoSystem7Reset();
			neoIPCAckCommand();
			break;
		case NEOARM7_PAUSE:
			neoSystem7Pause();
			break;
		case NEOARM7_BACKLIGHTOFF:
			neoBacklightOff();
			neoIPCAckCommand();
			break;
		case NEOARM7_BACKLIGHTON:
			neoBacklightOn();
			neoIPCAckCommand();
			break;
		case NEOARM7_RESUME:
		default:
			//meaningless, but safe to ignore
			neoIPCAckCommand();
			break;
		}
		command = neoIPCRecvCommand();
	}
}

void neoSystem7Init()
{
	g_neo7 = &g_neoContext7;

	neoAudioInit();
	neoYM2610Init();
	neoZ80Init();
}

void neoSystem7Reset()
{
	neoAudioReset();
	neoYM2610Init();
	neoZ80Reset();
}

void neoSystem7Execute()
{
	s32 cycles = 0;
	u32 i;

	while(1) {
		for(i = 0; i < Z80_TIMESLICE_PER_FRAME; i++) {
			cycles += Z80_CLOCKS_PER_TIMESLICE;
			cycles -= neoZ80Execute(cycles);
			neoSystem7ProcessCommands();
			neoYM2610Process();
		}
		swiIntrWait(1, IRQ_VCOUNT);
		neoAudioUpdate();
		//neoYM2610StreamProcess();
		NEOIPC->arm7Alive++;
	}
}

void systemPanic()
{
	REG_IME = 0;
	neoAudioPause();
	while(1) continue;
}

