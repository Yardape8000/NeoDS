#define NEO_IN_ASM
#include "NeoSystemAsm.h"

	.global neoVideoReadPal16
	.global neoVideoWritePal8
	.global neoVideoWritePal16
	.global neoSpriteTransfer

neoVideoWritePal16:
	//calculate offset into array
	mov r0, r0, lsl #19
	mov r0, r0, lsr #19 //r0 &= 0x1fff
	ldr r2, [r7, #NEO_PALETTEBANK]
	add r0, r0, r2, lsl #1 //r0 = offset
	
	//store data
	ldr r2, [r7, #NEO_PALETTE]
	strh r1, [r2, r0]
	
	//update dirty
	//divide by 32, now an index for which 16 color palette
	mov r0, r0, lsr #5
	//calculate byte offset
	add r3, r7, r0, lsr #3
	//load dirty byte
	ldrb r2, [r3, #NEO_PALETTEDIRTY]
	//r0 is index within byte
	and r0, r0, #0x7
	mov r1, #1
	mov r0, r1, lsl r0
	orr r2, r2, r0
	strb r2, [r3, #NEO_PALETTEDIRTY]
	
	bx lr
	
neoVideoWritePal8: //need to mask off low address bit
	//calculate offset into array
	mov r3, r0, lsl #19
	mov r3, r3, lsr #19 //r3 &= 0x1fff
	ldr r2, [r7, #NEO_PALETTEBANK]
	add r3, r3, r2, lsl #1 //r3 = offset
	
	ldr r2, [r7, #NEO_PALETTE]
	ldrh r3, [r2, r3]
	tst r0, #1
	beq 0f
	//odd
	and r3, r3, #0xff00
	orr r1, r3, r1
	b neoVideoWritePal16
0:
	//even
	and r3, r3, #0x00ff
	orr r1, r3, r1, lsl #8
	b neoVideoWritePal16
	
neoVideoReadPal16:
	//calculate offset into array
	mov r3, r0, lsl #19
	mov r3, r3, lsr #19 //r3 &= 0x1fff
	ldr r2, [r7, #NEO_PALETTEBANK]
	add r3, r3, r2, lsl #1 //r3 = offset
	
	ldr r2, [r7, #NEO_PALETTE]
	ldrh r0, [r2, r3]
	bx lr
	
//typedef struct PACKED _TSpriteTransferEntry {
//	void* pSave;
//	void* pDst;
//	const void* pSrc;
//} TSpriteTransferEntry;
//void neoSpriteTransfer(TSpriteTransferEntry* pTransfer, u32 transferCount);
/*	.arm
neoSpriteTransfer:
	cmp r1, #0
	bxeq lr
	stmdb sp!, {r4-r11, lr}
neoSpriteTransferLoop:
	str r1, [sp, #-4]!
	ldmia r0!, {r2, r3} //load pSave and pDst
	cmp r2, #0
	beq neoSpriteSwap //swap if pSave == 0
	//pDst -> pSave
	mov r14, #5
0:
	ldmia r3!, {r1, r4-r12} //40 bytes
	stmia r2!, {r1, r4-r12}
	ldmia r3!, {r1, r4-r12} //40 bytes
	stmia r2!, {r1, r4-r12}
	ldmia r3!, {r1, r4-r12} //40 bytes
	stmia r2!, {r1, r4-r12}
	ldmia r3!, {r1, r4-r12} //40 bytes
	stmia r2!, {r1, r4-r12}
	ldmia r3!, {r1, r4-r12} //40 bytes
	stmia r2!, {r1, r4-r12}
	subs r14, r14, #1
	bne 0b
	ldmia r3!, {r4-r9} //24 bytes
	stmia r2!, {r4-r9}
	
	//pSrc -> pDst
	sub r3, r3, #1024 //restore pDst
	ldr r2, [r0], #4 //load pSrc
	mov r14, #5
0:
	ldmia r2!, {r1, r4-r12} //40 bytes
	stmia r3!, {r1, r4-r12}
	ldmia r2!, {r1, r4-r12} //40 bytes
	stmia r3!, {r1, r4-r12}
	ldmia r2!, {r1, r4-r12} //40 bytes
	stmia r3!, {r1, r4-r12}
	ldmia r2!, {r1, r4-r12} //40 bytes
	stmia r3!, {r1, r4-r12}
	ldmia r2!, {r1, r4-r12} //40 bytes
	stmia r3!, {r1, r4-r12}
	subs r14, r14, #1
	bne 0b
	ldmia r2!, {r4-r9} //24 bytes
	stmia r3!, {r4-r9}
	
	ldr r1, [sp], #4
	subs r1, r1, #1
	bne neoSpriteTransferLoop
	b neoSpriteTransferDone
neoSpriteSwap:
	//swap pSrc, pDst
	ldr r2, [r0], #4 //load pSrc
	
	str r0, [sp, #-4]!
	mov r14, #6
0:
	str r14, [sp, #-4]!
	
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	ldmia r2, {r0-r1, r4-r7} //20 bytes
	ldmia r3, {r8-r12, r14} //20 bytes
	stmia r3!, {r0-r1, r4-r7}
	stmia r2!, {r8-r12, r14}
	
	ldr r14, [sp], #4
	subs r14, r14, #1
	bne 0b
	//now swap final 24 bytes
	ldmia r2, {r4-r7} //16 bytes
	ldmia r3, {r8-r11} //16 bytes
	stmia r3!, {r4-r7}
	stmia r2!, {r8-r11}
	
	ldr r0, [sp], #4
	ldr r1, [sp], #4
	subs r1, r1, #1
	bne neoSpriteTransferLoop
neoSpriteTransferDone:
	ldmfd sp!, {r4-r11, lr}
	bx lr*/

/*
u16 neoVideoReadPal16(u32 a)
{
	return g_neo->pPalette[((a >> 1) & 0x0fff) + g_neo->paletteBank];
}

void neoVideoWritePal8(u32 a, u8 d)
{
	u16 word = g_neo->pPalette[((a >> 1) & 0x0fff) + g_neo->paletteBank];
	if(a & 1) word = (word & 0xff00) | (u16)d;
	else word = (word & 0x00ff) | ((u16)d << 8);

	neoVideoWritePal16(a, word);
}

void neoVideoWritePal16(u32 a, u16 d)
{
	//const u16 r = ((d >> 7) & 0x1e) | ((d >> 14) & 0x01);
	//const u16 g = ((d >> 3) & 0x1e) | ((d >> 13) & 0x01);
	//const u16 b = ((d << 1) & 0x1e) | ((d >> 12) & 0x01);
	const u16 entry =
		((d >> 7) & 0x1e) | ((d >> 14) & 0x01) |
		((d << 2) & (0x1e << 5)) | ((d >> 8) & (0x01 << 5)) |
		((d << 11) & (0x1e << 10)) | ((d >> 2) & (0x01 << 10));

	const u32 address = (a & 0x1fff);
	const u32 offset = (address >> 1) + g_neo->paletteBank;
	g_neo->pNitroPalette[offset] = entry;//(b << 10) | (g << 5) | r;
	g_neo->pPalette[offset] = d;
	g_neo->paletteDirty |= 1 << (offset >> 8);
}
*/
/*mov r3, r1, lsr #7
	and r3, r3, #0x1e
	
	mov r2, r1, lsr #14
	and r2, r2, #0x01
	orr r3, r3, r2
	
	mov r2, r1, lsl #2
	and r2, r2, #(0x1e << 5)
	orr r3, r3, r2
	
	mov r2, r1, lsr #8
	and r2, r2, #(0x01 << 5)
	orr r3, r3, r2
	
	mov r2, r1, lsl #11
	and r2, r2, #(0x1e << 10)
	orr r3, r3, r2
	
	mov r2, r1, lsr #2
	and r2, r2, #(0x01 << 10)
	orr r3, r3, r2*/
