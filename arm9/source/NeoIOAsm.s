#define NEO_IN_ASM
#include "NeoSystemAsm.h"
#include "NeoIPC.h"
	
	.global neoIOInit
	
	.global neoReadCtrl116
	.global neoReadCtrl216
	.global neoReadCtrl316
	.global neoReadCoin16
	
	.global neoWriteVideo8
	.global neoWriteVideo16
	.global neoWriteVideo32
	
	.global neoReadVideo8
	.global neoReadVideo16
	.global neoReadVideo32
	
	.global neoWriteWatchdog8
	.global neoWriteWatchdog16

	.arm
	.align 4
	.section .text
neoIOInit:
	ldr r0, =neoWriteVideo_vramOffset
	str r0, [r7, #NEO_VIDEOWRITETABLE+0]
	
	ldr r0, =neoWriteVideo_vramData
	str r0, [r7, #NEO_VIDEOWRITETABLE+4]
	
	ldr r0, =neoWriteVideo_vramMod
	str r0, [r7, #NEO_VIDEOWRITETABLE+8]
	
	ldr r0, =neoWriteVideo_controlReg
	str r0, [r7, #NEO_VIDEOWRITETABLE+12]
	
	ldr r0, =neoWriteVideo_irq2Pos1
	str r0, [r7, #NEO_VIDEOWRITETABLE+16]
	
	ldr r0, =neoWriteVideo_irq2Pos2
	str r0, [r7, #NEO_VIDEOWRITETABLE+20]
	
	ldr r0, =neoWriteVideo_irkAk
	str r0, [r7, #NEO_VIDEOWRITETABLE+24]
	
	ldr r0, =cpuNullWrite16
	str r0, [r7, #NEO_VIDEOWRITETABLE+28]
	
	//wrap to beginning
	ldr r0, =neoWriteVideo_vramOffset
	str r0, [r7, #NEO_VIDEOWRITETABLE+32]
	
	bx lr
	
	.ltorg


	.arm
	.align 4
	.section .itcm
neoWriteVideo16:
	add r2, r7, #NEO_VIDEOWRITETABLE
	and r0, r0, #0x0e
	ldr pc, [r2, r0, lsl #1]
	
neoWriteVideo8:
	tst r0, #0x01
	bxne lr
	add r2, r7, #NEO_VIDEOWRITETABLE
	and r0, r0, #0x0e
	orr r1, r1, r1, lsl #8
	ldr pc, [r2, r0, asl #1]
	
neoWriteVideo32:
	str lr, [sp, #-4]!
	
	add r2, r7, #NEO_VIDEOWRITETABLE
	and r0, r0, #0x0e
	
	//high word
	add lr, pc, #4
	mov r1, r1, ror #16
	ldr pc, [r2, r0, lsl #1]!
	
	//low word
	ldr lr, [sp], #4
	mov r1, r1, ror #16
	ldr pc, [r2, #4]
	
neoWriteVideo_vramOffset:
	tst r1, #0x8000
	beq 0f
	
	mov r3, #0x1000
	sub r3, r3, #2
	str r3, [r7, #NEO_VRAMMASK]
	
	and r3, r3, r1, lsl #1 //mask initial offset
	str r3, [r7, #NEO_VRAMOFFSET]
	
	ldr r3, [r7, #NEO_SPRITERAM]
	str r3, [r7, #NEO_VRAMBASE]
	bx lr
0:
	mov r3, #0x10000
	sub r3, r3, #2
	str r3, [r7, #NEO_VRAMMASK]
	
	and r3, r3, r1, lsl #1 //mask initial offset
	str r3, [r7, #NEO_VRAMOFFSET]
	
	ldr r3, [r7, #NEO_VRAM]
	str r3, [r7, #NEO_VRAMBASE]
	bx lr

neoWriteVideo_vramData:
	ldr r0, [r7, #NEO_VRAMBASE]
	ldr r3, [r7, #NEO_VRAMOFFSET]
	strh r1, [r0, r3]
	ldr r0, [r7, #NEO_VRAMMOD]
	add r3, r3, r0 //offset += mod
	ldr r0, [r7, #NEO_VRAMMASK]
	and r3, r3, r0 //offset &= mask
	str r3, [r7, #NEO_VRAMOFFSET]
	bx lr
	
neoWriteVideo_vramMod:
	mov r3, r1, lsl #16
	mov r3, r3, lsr #15
	str r3, [r7, #NEO_VRAMMOD]
	bx lr

neoWriteVideo_controlReg:
	mov r3, r1, lsl #16
	mov r3, r3, lsr #16
	str r3, [r7, #NEO_DISPLAYCONTROL]
	
	//stmdb sp!, {r0-r11,lr}
	//mov r0, r3
	//ldr r1, =neoDebugControlReg
	//blx r1
	//ldmia sp!, {r0-r11,lr}
	
	
	//add mask bits
	ldr r0, [r7, #NEO_DISPLAYCONTROLMASK]
	orr r0, r0, r3
	str r0, [r7, #NEO_DISPLAYCONTROLMASK]
	bx lr

neoWriteVideo_irq2Pos1:
	ldr r3, [r7, #NEO_DISPLAYCOUNTERLOAD]
	mov r3, r3, lsl #16
	mov r3, r3, lsr #16
	orr r3, r3, r1, lsl #16
	str r3, [r7, #NEO_DISPLAYCOUNTERLOAD]
	bx lr

neoWriteVideo_irq2Pos2:
	ldr r3, [r7, #NEO_DISPLAYCOUNTERLOAD]
	mov r0, r1, lsl #16 //chop off upper half of r1
	mov r3, r3, lsr #16 //chop off lower half of r3
	mov r0, r0, lsr #16 //move back to lower half
	orr r3, r0, r3, lsl #16 //move back to upper half
	str r3, [r7, #NEO_DISPLAYCOUNTERLOAD]
	//see if need to reload display counter
	ldr r0, [r7, #NEO_DISPLAYCONTROL]
	tst r0, #(1 << 5)
	strne r3, [r7, #NEO_DISPLAYCOUNTER]
	bx lr
	
neoWriteVideo_irkAk:
	stmdb sp!, {r0-r3, lr}
	mov r0, r1 //pass data to neoSystemIrqAk
	bl neoSystemIrqAk
	ldmfd sp!, {r0-r3, lr}
	bx lr
	
	.ltorg
	
	.section .rodata
neoReadVideoTable:
	.long neoReadVideo_vramOffset
	.long neoReadVideo_vramData
	.long neoReadVideo_vramMod
	.long neoReadVideo_controlReg
	.long neoReadVideo_vramData
	.long neoReadVideo_vramData
	.long neoReadVideo_vramMod
	.long neoReadVideo_vramData //should be null?
	.long neoReadVideo_vramOffset //extra entry (for 32 bit wrap)

	.section .text
neoReadVideo16:
	ldr r2, =neoReadVideoTable
	and r0, r0, #0x0e
	ldr pc, [r2, r0, lsl #1]
	
neoReadVideo8: //doesn't return proper thing if unmapped
	//odd address are invalid
	tst r0, #0x01
	bxne lr
	
	str lr, [sp], #-4
	ldr r2, =neoReadVideoTable
	add lr, pc, #4
	and r0, r0, #0x0e
	ldr pc, [r2, r0, asl #1]
	//return high byte read
	mov r0, r0, lsr #8
	ldr pc, [sp, #4]!
	
neoReadVideo32:
	stmdb sp!, {r8, r9, lr}
	
	ldr r2, =neoReadVideoTable
	and r0, r0, #0x0e
	add r8, r2, r0, lsl #1 //address of jumptable entry
	
	//read high word first
	add lr, pc, #4
	ldr pc, [r8], #4 //advance to next entry (table has an extra entry for this case)
	mov r9, r0, lsl #16 //save high word in r9
	
	//now read low word
	mov r2, r8
	add lr, pc, #4
	ldr pc, [r2]
	add r0, r0, r9 //add high word onto low word
	
	ldmfd sp!, {r8, r9, lr}
	bx lr
	
	.ltorg
	
neoReadVideo_vramOffset:
	ldr r0, [r7, #NEO_VRAMOFFSET]
	mov r0, r0, lsr #1
	bx lr
	
neoReadVideo_vramData:
	ldr r2, [r7, #NEO_VRAMOFFSET]
	ldr r0, [r7, #NEO_VRAMBASE]
	//mov r2, r2, lsl #1
	ldrh r0, [r0, r2]
	bx lr
	
neoReadVideo_vramMod:
	ldr r0, [r7, #NEO_VRAMMOD]
	mov r0, r0, lsr #1
	bx lr
	
neoReadVideo_controlReg:
	//return ((248 + g_neo->scanline) << 7) | g_neo->autoAnimationCounter;
	//str lr, [sp, #-4]!
	//mov r0, r5 //pass "cycles to function
	//bx =neoGetScanline
	//ldr lr, [sp], #4
	
	ldr r0, [r7, #NEO_SCANLINE]
	add r0, r0, #248
	ldr r1, [r7, #NEO_AUTOANIMCOUNTER]
	add r0, r1, r0, lsl #7
	bx lr
	.ltorg
	
neoWriteWatchdog16:
	mov r1, #0
	str r1, [r7, #NEO_WATCHDOG]
	bx lr
	
neoWriteWatchdog8:
	mov r1, #0
	tst r0, #1
	strne r1, [r7, #NEO_WATCHDOG] //only odd writes reset
	bx lr
	
neoReadCtrl116:
	and r0, r0, #0xff
	cmp r0, #0x00
	beq 0f
	cmp r0, #0x01
	beq 0f
	
	cmp r0, #0x80
	beq 1f
	cmp r0, #0x81
	beq 1f
	
	//nothing
	mov r0, #0
	bx lr
0:
	//high byte is joystick
	//low byte is dips
	ldr r0, [r7, #NEO_CTRL1REG]
	bx lr
1:
	//high byte is ?
	//low byte is test stuff
	//return 0xff80;
	ldr r0, [r7, #NEO_CTRL4REG]
	bx lr

neoReadCtrl216:
	ldr r0, [r7, #NEO_CTRL2REG]
	bx lr
	
neoReadCtrl316:
	//high byte is start buttons and memcard status
	//low byte is nothing
	ldr r0, [r7, #NEO_CTRL3REG]
	bx lr
	
neoReadCoin16:
	ldr r0, [r7, #NEO_COINREG]
	mov r1, #0
	ldr r2, =NEOIPC_AUDIORESULT
	ldrb r1, [r2]
	orr r0, r0, r1, lsl #8
	//clear high bit if command pending
	ldr r2, =NEOIPC_AUDIOCOMMANDPENDING
	ldrb r1, [r2]
	cmp r1, #0
	bicne r0, r0, #0x8000
	bx lr
	
	.ltorg

/*
u16 neoReadCtrl116(u32 a)
{
	u16 data;
	switch(a) {
	case 0x300000: //IN0 - ctrl1
	case 0x300001:
		//high byte is joystick
		//low byte is dips
		//bit0F - button 4
		//bit0E - button 3
		//bit0D - button 2
		//bit0C - button 1
		//bit0B - button right
		//bit0A - button left
		//bit09 - button down
		//bit08 - button up
		//bit07
		//bit06
		//bit05
		//bit04
		//bit03
		//bit02
		//bit01
		//bit00 - test switch
		data = 0x00ff | ((u16)g_neo->p1Input << 8);
		return data;
	case 0x300080: //IN4
	case 0x300081:
		//high byte is ?
		//low byte is test stuff
		return 0xff80;
	default:
		return 0x0000;
	}
}

u16 neoReadCoin16(u32 a) //coin
{
	//high byte is audio result
	//low byte is other stuff
	const u16 testbit = read_4990_testbit();
	const u16 databit = read_4990_databit();
	u16 data = 0x0104 | (testbit << 6) | (databit << 7) | g_neo->coinInput;
	//if(g_neo->keys & KEY_SELECT) data &= ~0x01;
	return data;
}

u16 neoReadCtrl316(u32 a) //ctrl3
{
	//high byte is start buttons and memcard status
	//low byte is nothing
	u16 data = 0x8A00 | 0x7000 | (g_neo->startInput << 8); //0x7000 is memcard not present
	//if(g_neo->keys & KEY_START) data &= ~(1 << 8);
	return data; 
}

void neoWriteWatchdog16(u32 a, u16 d)
{
	g_neo->watchdogCounter = 0;
}

u16 neoReadVideo16(u32 a)
{
	switch(a & 0x0e) {
	case 0x00: //vram offset
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x02: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x04: //vram mod
		return g_neo->vramMod;
	case 0x06: //control reg
		return ((248 + g_neo->scanline) << 7) | g_neo->autoAnimationCounter;
	case 0x08: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x0A: //vram data
		return g_neo->pVramBase[g_neo->vramOffset];
	case 0x0C: //vram mod
		return g_neo->vramMod;
	}
	return cpuUnmapped16();
}


void neoWriteVideo16(u32 a, u16 d)
{
	switch(a & 0x0e) {
	case 0x00: //vram offset
		if(d < 0x8000) {
			g_neo->pVramBase = g_neo->pVram;
			g_neo->vramBaseMask = 0x7fff;
			g_neo->vramOffset = d;
		} else {
			g_neo->pVramBase = g_neo->pSpriteRam;
			g_neo->vramBaseMask = 0x7ff;
			g_neo->vramOffset = d & 0x7ff;
		}
		break;
	case 0x02: //vram data
		g_neo->pVramBase[g_neo->vramOffset] = d;
		g_neo->vramOffset =
			((g_neo->vramOffset + g_neo->vramMod) & g_neo->vramBaseMask);
		break;
	case 0x04: //vram mod
		g_neo->vramMod = d;
		break;
	case 0x06: //control reg
		g_neo->displayControl = d;
		if(d & (1 << 5)) {
			g_neo->displayCounter = g_neo->displayCounterLoad;
		}
		break;
	case 0x08: //irq2 pos1
		g_neo->displayCounterLoad &= 0x0000ffff;
		g_neo->displayCounterLoad |= (u32)d << 16;
		break;
	case 0x0A: //irq2 pos2
		g_neo->displayCounterLoad &= 0xffff0000;
		g_neo->displayCounterLoad |= (u32)d;
		break;
	case 0x0C: //irq ak
		neoSystemIrqAk(d);
		break;
	}
}*/
