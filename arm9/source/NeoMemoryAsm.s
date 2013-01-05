#define NEO_IN_ASM
#include "NeoSystemAsm.h"

	.global neoWriteSram8
	.global neoWriteSram16
	.global neoWriteSram32
	
	.global neoReadSram8
	.global neoReadSram16
	.global neoReadSram32
	
	.global neoReadSlot2Rom8
	.global neoReadSlot2Rom16
	.global neoReadSlot2Rom32
	.global neoSlot2RomPc
	
	.global neoReadBankedRom8
	.global neoReadBankedRom16
	.global neoReadBankedRom32
	
	.global neoReadBankedRom8Uncached
	.global neoReadBankedRom16Uncached
	.global neoReadBankedRom32Uncached
	.global neoBankedPcUncached

	.arm
	.section .text
neoWriteSram8:
	//do nothing if latch is 0
	ldrb r2, [r7, #NEO_SRAMLATCH]
	tst r2, #1
	bxeq lr
	
	//address = (address & 0xffff) ^ 1
	eor r0, r0, #1
	mov r0, r0, lsl #16
	mov r0, r0, lsr #16
	
	//check if data == 0
	cmp r1, #1
	bne 0f
	
	//if address also == sram protect address, do nothing
	ldr r2, [r7, #NEO_SRAMPROTECT]
	cmp r2, r0
	bxeq lr
0:
	//finally do write
	ldr r2, [r7, #NEO_SRAM]
	strb r1, [r2, r0]
	bx lr
	
neoWriteSram16:
	//do nothing if latch is 0
	ldrb r2, [r7, #NEO_SRAMLATCH]
	tst r2, #1
	bxeq lr
	
	//address = (address & 0xffff)
	mov r0, r0, lsl #16
	mov r0, r0, lsr #16
	
	//check if data == 0
	cmp r1, #1
	bne 0f
	
	//if address also == sram protect address, do nothing
	ldr r2, [r7, #NEO_SRAMPROTECT]
	cmp r2, r0
	bxeq lr
0:
	//finally do write
	ldr r2, [r7, #NEO_SRAM]
	strh r1, [r2, r0]
	bx lr
	
/*neoWriteSram32:
	mov r3, lr //save old lr
	mov r1, r1, ror #16 //write high word first
	bl neoWriteSram16
	
	mov r1, r1, ror #16 //now write low word
	add r0, r0, #2 //advance address
	mov lr, r3 //restore old lr
	b neoWriteSram16 // do tail call*/
	
neoReadSram8:
	eor r0, r0, #1
	ldr r2, [r7, #NEO_SRAM]
	mov r0, r0, lsl #16
	ldrb r0, [r2, r0, lsr #16]
	bx lr
	
neoReadSram16:
	mov r0, r0, lsl #16
	ldr r2, [r7, #NEO_SRAM]
	mov r0, r0, lsr #16
	ldrh r0, [r2, r0]
	bx lr
	
neoReadSram32:
	mov r0, r0, lsl #16
	ldr r2, [r7, #NEO_SRAM]
	mov r0, r0, lsr #16
	ldrh r1, [r2, r0]!
	ldrh r0, [r2, #2]
	add r0, r0, r1, lsl #16
	bx lr
	
	.arm
	.section .itcm
neoReadSlot2Rom8:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	eor r0, r0, #1
	ldrb r0, [r2, r0]
	bx lr
	
neoReadSlot2Rom16:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	ldrh r0, [r2, r0]
	bx lr
	
neoReadSlot2Rom32:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	ldrh r1, [r2, r0]!
	ldrh r0, [r2, #2]
	add r0, r0, r1, lsl #16
	bx lr
	
neoSlot2RomPc:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	add r0, r0, r2
	str r2, [r7, #NEO_CPU_MEMBASE]
	bx lr
	
	//in: r1 = cacheEntryIndex
	//out: r2 = pEntry
	.arm
	.section .text
neoEvictAndLoadBank:
	stmdb sp!, {r0, r1, r3, r12, lr}
	str r1, [sp, #-4]! //save bankIndex
	
	ldr r2, =neoEvictBank
	blx r2
	
	mov r1, r0 //move cacheIndex into place
	ldr r0, [sp], #4 //restore cacheEntryIndex into proper location
	
	ldr r2, =neoLoadBank
	blx r2
	mov r2, r0 //store return value in r2
	
	ldmfd sp!, {r0, r1, r3, r12, lr}
	bx lr
	
	.ltorg
	
neoEvictAndLoadBank8:
	str lr, [sp, #-4]! //save old link register
	bl neoEvictAndLoadBank
	ldr lr, [sp], #4 //restore link register
	b neoReadBankedRom8Return

neoEvictAndLoadBank16:
	str lr, [sp, #-4]! //save old link register
	bl neoEvictAndLoadBank
	ldr lr, [sp], #4 //restore link register
	b neoReadBankedRom16Return
	
neoEvictAndLoadBank32a:
	str lr, [sp, #-4]! //save old link register
	bl neoEvictAndLoadBank
	ldr lr, [sp], #4 //restore link register
	b neoReadBankedRom32aReturn
	
neoEvictAndLoadBank32b:
	str lr, [sp, #-4]! //save old link register
	bl neoEvictAndLoadBank
	ldr lr, [sp], #4 //restore link register
	b neoReadBankedRom32bReturn

	.arm
	.section .itcm
neoReadBankedRom8:
	ldr r1, [r7, #NEO_ROMBANK]
	ldr r3, [r7, #NEO_BANKTABLE]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	mov r1, r0, lsr #BANK_CACHE_SHIFT
	ldr r2, [r3, r1, lsl #2]
	cmp r2, #BANK_CACHE_INVALID_ASM
	beq neoEvictAndLoadBank8
neoReadBankedRom8Return:
	eor r0, r0, #1 //byte swap
	mov r0, r0, lsl #(32 - BANK_CACHE_SHIFT) //mask off upper bits
	ldrb r0, [r2, r0, lsr #(32 - BANK_CACHE_SHIFT)]
	bx lr
	
	.arm
	.section .itcm
neoReadBankedRom16:
	ldr r1, [r7, #NEO_ROMBANK]
	ldr r3, [r7, #NEO_BANKTABLE]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	mov r1, r0, lsr #BANK_CACHE_SHIFT
	ldr r2, [r3, r1, lsl #2]
	cmp r2, #BANK_CACHE_INVALID_ASM
	beq neoEvictAndLoadBank16
neoReadBankedRom16Return:
	mov r0, r0, lsl #(32 - BANK_CACHE_SHIFT) //mask off upper bits
	mov r0, r0, lsr #(32 - BANK_CACHE_SHIFT) //mask off upper bits
	ldrh r0, [r2, r0]
	bx lr
	
	.arm
	.section .itcm
	//NOTE - uses r12
neoReadBankedRom32:
	ldr r1, [r7, #NEO_ROMBANK]
	ldr r3, [r7, #NEO_BANKTABLE]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	mov r1, r0, lsr #BANK_CACHE_SHIFT
	ldr r2, [r3, r1, lsl #2]
	cmp r2, #BANK_CACHE_INVALID_ASM
	beq neoEvictAndLoadBank32a
neoReadBankedRom32aReturn:
	mov r1, r0, lsl #(32 - BANK_CACHE_SHIFT) //mask off upper bits
	mov r1, r1, lsr #(32 - BANK_CACHE_SHIFT) //mask off upper bits
	ldrh r12, [r2, r1]
	
	add r1, r1, #2
	tst r1, #(1 << BANK_CACHE_SHIFT)
	bne 0f
	//we are in same cache page
	ldrh r0, [r2, r1]
	add r0, r0, r12, lsl #16
	bx lr
0:
	//we moved to the next cache page
	add r0, r0, #2 //goto next address
	mov r1, r0, lsr #BANK_CACHE_SHIFT
	ldr r2, [r3, r1, lsl #2]
	cmp r2, #BANK_CACHE_INVALID_ASM
	beq neoEvictAndLoadBank32b
neoReadBankedRom32bReturn:
	//at the start of page, don't need offset
	ldrh r0, [r2]
	add r0, r0, r12, lsl #16
	bx lr
	
	.section .text
neoReadBankedRom8Uncached:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	
	eor r0, r0, #1
	ldr r2, [r7, #NEO_ROM1]
	ldrb r0, [r2, r0]
	bx lr
	
neoReadBankedRom16Uncached:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	ldrh r0, [r2, r0]
	bx lr
	
neoReadBankedRom32Uncached:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	ldrh r1, [r2, r0]!
	ldrh r0, [r2, #2]
	add r0, r0, r1, lsl #16
	bx lr
	
neoBankedPcUncached:
	ldr r1, [r7, #NEO_ROMBANK]
	add r0, r1, r0
	bic r0, r0, #0xff000000
	ldr r2, [r7, #NEO_ROM1]
	sub r1, r2, #0x200000
	add r0, r2, r0
	str r1, [r7, #NEO_CPU_MEMBASE]
	bx lr	

/*
	const u32 address = a + g_neo->romBankAddress;
	const u32 cacheEntryIndex =
		(address >> BANK_CACHE_SHIFT) & (BANK_TABLE_SIZE - 1);
	const u8* pEntry = g_neo->bankTable[cacheEntryIndex];
	if(pEntry == BANK_CACHE_INVALID) {
		u32 cacheIndex = neoEvictBank();
		pEntry = neoLoadBank(cacheEntryIndex, cacheIndex);
	}
	return CPU_READ8(pEntry, address & (BANK_CACHE_ENTRY_SIZE - 1));
*/

