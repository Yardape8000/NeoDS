#include <nds.h>
#include <stdlib.h>
#include "NeoSystem7.h"
#include "NeoCpuZ80.h"
#include "NeoYM2610.h"
#include "NeoIPC.h"
#include "NeoAudio.h"

//static int vcount = 80;
//static touchPosition first,tempPos;

static void handleInput()
{
	/*static int lastbut = -1;
	
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {
 
		tempPos = touchReadXY();

		if ( tempPos.x == 0 || tempPos.y == 0 ) {
			but |= (1 <<6);
			lastbut = but;
		} else {
			x = tempPos.x;
			y = tempPos.y;
			xpx = tempPos.px;
			ypx = tempPos.py;
			z1 = tempPos.z1;
			z2 = tempPos.z2;
		}
		
	} else {
		lastbut = but;
		but |= (1 <<6);
	}

	if ( vcount == 80 ) {
		first = tempPos;
	} else {
		if (	abs( xpx - first.px) > 10 || abs( ypx - first.py) > 10 ||
				(but & ( 1<<6)) ) {

			but |= (1 <<6);
			lastbut = but;

		} else { 	
			IPC->mailBusy = 1;
			IPC->touchX			= x;
			IPC->touchY			= y;
			IPC->touchXpx		= xpx;
			IPC->touchYpx		= ypx;
			IPC->touchZ1		= z1;
			IPC->touchZ2		= z2;
			IPC->mailBusy = 0;
		}
	}
	IPC->buttons		= but;
	vcount ^= (80 ^ 130);*/
}

static void VcountHandler()
{
	handleInput();
}

static void VblankHandler()
{

}

static void IPCSyncHandler()
{

}

//from DrZ80.asm
//extern u32 DAATABLE_LOCAL;

int main(int argc, char ** argv)
{
	neoIPCInit();
	//Reset the clock if needed
	rtcReset();

	irqInit();
	irqSet(IRQ_VBLANK, VblankHandler);
	SetYtrigger(0);
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_IPC_SYNC, IPCSyncHandler);
	irqSet(IRQ_TIMER3, neoAudioEventHandler);
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_IPC_SYNC | IRQ_TIMER3);
	REG_IME = 1;

	//wait for arm9 to reset us
	neoIPCWaitCommand(NEOARM7_RESET);

	//kinda messy...we keep the DaaTable in main ram on the arm9 side,
	//give the arm7 a pointer through the IPC struct here
	//DAATABLE_LOCAL = (u32)NEOIPC->pZ80DaaTable;
	
	neoSystem7Init();

	neoIPCAckCommand();

	neoSystem7Execute();

	return 0;
}


