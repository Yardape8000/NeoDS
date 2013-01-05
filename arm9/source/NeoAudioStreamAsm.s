#define NEO_IN_ASM
#include "NeoSystemAsm.h"

	//.global neoAudioAdpcmaDecode

	.arm
	.align 2
neoAudioAdpcmaDecode:
	cmp r2, #0
	bxeq lr
	
	stmdb sp!, {r4-r9}
	ldrsh r4, [r3, #8] //acc
	ldrsh r5, [r3, #10] //step
	ldr r9, =g_jediTable
0:
	ldrb r6, [r1], #1 //data = *pSrc++
	and r7, r6, #0xf0 //data &= 0xf0
	add r8, r9, r7, lsr #2 //pJedi = &g_jediTable[data >> 2]
	//load both values from table
	ldr r7, [r8, r5, asl #1]
	//acc is in low 16 bits
	mov r8, r7, lsl #16
	add r4, r4, r8, asr #16
	//step is in high 16 bits
	mov r5, r7, asr #16
	//store acc
	strh r4, [r0], #2
	
	and r7, r6, #0x0f
	add r8, r9, r7, lsl #2
	//load both values from table
	ldr r7, [r8, r5, asl #1]
	//acc is in low 16 bits
	mov r8, r7, lsl #16
	add r4, r4, r8, asr #16
	//step is in high 16 bits
	mov r5, r7, asr #16
	//store acc
	strh r4, [r0], #2
	
	subs r2, r2, #1
	bne 0b
	
	strh r4, [r3, #8]
	strh r5, [r3, #10]
	
	ldmfd sp!, {r4-r9}
	bx lr
	
	.ltorg

/*static void ITCM_CODE neoAudioAdpcmaDecode(u16* restrict pDst, const u8* restrict pSrc,
										   u32 length, TNeoADPCMStream* pAdpcm)
{
	const s32* restrict pJedi;
	s32 acc = pAdpcm->acc;
	s32 step = pAdpcm->step;
	s32 i;

	for(i = length; i > 0; i--) {
		const u32 data = *pSrc++;
		
		//the data nibble has an extra left shift by 1 tacked on (>> 3 rather than >> 4)
		pJedi = &g_jediTable[step + ((data >> 3) & 0x1e)];
		acc += *pJedi++;
		step = *pJedi;
		*pDst++ = acc;
		
		pJedi = &g_jediTable[step + ((data << 1) & 0x1e)];
		acc += *pJedi++;
		step = *pJedi;
		*pDst++ = acc;
	}

	pAdpcm->acc = acc;
	pAdpcm->step = step;
}*/
