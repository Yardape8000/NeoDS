#define NEO_IN_ASM
#include "NeoSystem7.h"

	.global neoZ80Read8
	.global neoZ80Read16
	.global neoZ80Write8
	.global neoZ80Write16

	.section .text
	.align 2
	.arm
neoZ80Read8:
	add r2, r5, #NEO_Z80MEMTABLE
	and r1, r0, #0x0000f800
	ldr r2, [r2, r1, lsr #(MEMBLOCK_SHIFT - 2)]
	mov r0, r0, lsl #(32 - MEMBLOCK_SHIFT)
	ldrb r0, [r2, r0, lsr #(32 - MEMBLOCK_SHIFT)]
	bx lr
	
neoZ80Read16:
	str r3, [sp, #-4]!
	
	//low byte
	add r2, r5, #NEO_Z80MEMTABLE
	and r1, r0, #0x0000f800
	ldr r2, [r2, r1, lsr #(MEMBLOCK_SHIFT - 2)]
	mov r1, r0, lsl #(32 - MEMBLOCK_SHIFT)
	ldrb r3, [r2, r1, lsr #(32 - MEMBLOCK_SHIFT)]
	
	//move address forward
	add r0, r0, #1
	
	//high byte
	add r2, r5, #NEO_Z80MEMTABLE
	and r1, r0, #0x0000f800
	ldr r2, [r2, r1, lsr #(MEMBLOCK_SHIFT - 2)]
	mov r1, r0, lsl #(32 - MEMBLOCK_SHIFT)
	ldrb r0, [r2, r1, lsr #(32 - MEMBLOCK_SHIFT)]
	
	//combine
	add r0, r3, r0, lsl #8
	ldr r3, [sp], #4
	bx lr
	
neoZ80Write8:
	cmp r1, #0xf800
	bxlo lr
	add r2, r5, #NEO_Z80RAM
	mov r1, r1, lsl #(32 - MEMBLOCK_SHIFT)
	strb r0, [r2, r1, lsr #(32 - MEMBLOCK_SHIFT)]
	bx lr
	
neoZ80Write16:
	cmp r1, #0xf800
	bxlo lr
	
	str r3, [sp, #-4]!
	
	//low byte
	add r2, r5, #NEO_Z80RAM
	mov r3, r1, lsl #(32 - MEMBLOCK_SHIFT)
	strb r0, [r2, r3, lsr #(32 - MEMBLOCK_SHIFT)]
	
	mov r0, r0, lsr #8 //move high byte down
	add r1, r1, #1 //move to next address
	
	//high byte
	mov r3, r1, lsl #(32 - MEMBLOCK_SHIFT)
	strb r0, [r2, r3, lsr #(32 - MEMBLOCK_SHIFT)]
	
	ldr r3, [sp], #4
	bx lr

/*
u8 neoZ80Read8(u16 a)
{
	const u8* pBase = g_neo7->z80MemTable[a >> MEMBLOCK_SHIFT];
	return pBase[a & (MEMBLOCK_SIZE - 1)];
}

u16 neoZ80Read16(u16 a)
{
	const u8* pBase = g_neo7->z80MemTable[a >> MEMBLOCK_SHIFT];
	u16 d = pBase[a & (MEMBLOCK_SIZE - 1)];
	
	a++;
	pBase = g_neo7->z80MemTable[a >> MEMBLOCK_SHIFT];
	return d | ((u16)pBase[a & (MEMBLOCK_SIZE - 1)] << 8);
}

void neoZ80Write8(u8 d, u16 a)
{
	if(a >= 0xf800) {
		g_neo7->z80Ram[a & 0x7ff] = d;
	}
}

void neoZ80Write16(u16 d, u16 a)
{
	if(a >= 0xf800) {
		g_neo7->z80Ram[a & 0x7ff] = (u8)d; //low byte first
		g_neo7->z80Ram[(a + 1) & 0x7ff] = (u8)(d >> 8); //high byte second
	}
	//a++;
	//if(a >= 0xf800) {
	//	g_neo7->z80Ram[a & 0x7ff] = (u8)(d >> 8); //high byte second
	//}
}
*/
