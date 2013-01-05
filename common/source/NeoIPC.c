#include "nds.h"
#include "NeoIPC.h"

#ifdef ARM9

#include "EmuSystem.h"
#include "NeoSystem.h"

STATIC_ASSERT(sizeof(TNeoIPC) <= 2048);

#else

#include "NeoSystem7.h"

#endif

#define NEOIPC_PACK_COMMAND(command, arg) \
	((command << NEOIPC_COMMAND_SHIFT) & NEOIPC_COMMAND_MASK) | \
	((arg << NEOIPC_ARG_SHIFT) & NEOIPC_ARG_MASK)

enum {
	IPCSTATE_IDLE,
	IPCSTATE_RECVD,
};

static u32 g_ipcState = IPCSTATE_IDLE;

static void neoIPCDelay()
{
	swiIntrWait(0, IRQ_IPC_SYNC);
}

void neoIPCInit()
{
	REG_IPC_FIFO_CR = IPC_FIFO_SEND_CLEAR | IPC_FIFO_ERROR | IPC_FIFO_ENABLE;
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
}

u32 neoIPCSendCommandAsync(TNeoIPCCommand command)
{
	//wait for room in command fifo
	while(REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL) continue;

	REG_IPC_FIFO_TX = NEOIPC_PACK_COMMAND(command, 0);
	IPC_SendSync(command);
	
#ifdef ARM9
	NEOIPC->arm9FifoSent++;
	//systemWriteLine("Sent command: %d (%d)", command, NEOIPC->arm9FifoSent);
	return NEOIPC->arm9FifoSent;
#else
	NEOIPC->arm7FifoSent++;
	return NEOIPC->arm7FifoSent;
#endif
}

void neoIPCSendCommand(TNeoIPCCommand command)
{
	const u32 message = neoIPCSendCommandAsync(command);
	neoIPCWaitCommandDone(message);
}


bool neoIPCCheckCommandDone(u32 message)
{
#ifdef ARM9
	const u32 processed = NEOIPC->arm7FifoProcessed;
#else
	const u32 processed = NEOIPC->arm9FifoProcessed;
#endif
	if(processed >= message) {
		return true;
	} else if(message - processed > 0x80000000) {
		// Assume wraparound if difference is large
		return true;
	} else {
		return false;
	}
}

void neoIPCWaitCommandDone(u32 message)
{
#ifdef ARM9
	//systemWriteLine("Waiting for index %d...", message);
#endif

	while(!neoIPCCheckCommandDone(message)) {
		neoIPCDelay();
#ifdef ARM9
		//systemWriteLine(" -> tick");
#endif
	}
#ifdef ARM9
	//systemWriteLine(" -> Done!");
#endif
}

u32 neoIPCRecvCommand()
{
	ASSERT(g_ipcState == IPCSTATE_IDLE);

	u32 command = 0;
	if(!(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY)) {
		//grab command off fifo if we have one
		command = REG_IPC_FIFO_RX;
		g_ipcState = IPCSTATE_RECVD;
	}

	return command;
}

u32 neoIPCWaitCommand(TNeoIPCCommand command)
{
	u32 data = 0;

#ifdef ARM9
	systemWriteLine("Waiting for %d...", command);
#endif

	while(1) {
		u32 data = neoIPCRecvCommand();
		if(NEOIPC_GET_COMMAND(data) == command) {
			break;
		} else if(data != 0) {
			//we got a valid command, but not the one we waiting for
			//just throw it away
#ifdef ARM9
			systemWriteLine(" -> Got %d...", data);
#endif
			neoIPCAckCommand();
		}
		neoIPCDelay();
	}

#ifdef ARM9
	systemWriteLine(" -> done!");
#endif

	return data;
}

void neoIPCAckCommand()
{
	ASSERT(g_ipcState == IPCSTATE_RECVD);
#ifdef ARM9
	NEOIPC->arm9FifoProcessed++;
#else
	NEOIPC->arm7FifoProcessed++;
#endif
	IPC_SendSync(0);
	g_ipcState = IPCSTATE_IDLE;
}
