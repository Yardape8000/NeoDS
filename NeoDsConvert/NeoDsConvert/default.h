#ifndef _DEFAULT_H
#define _DEFAULT_H

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef UINT32 FPTR;

//mameHelper
UINT8* memory_region(int region);
UINT8* malloc_or_die(int size);
void* auto_malloc(int size);
int memory_region_length(int region);

#define state_save_register_global_pointer(a, b) ((void)0)

extern int neogeo_fixed_layer_bank_type;

/* memory region types */
enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_DISKS,
	REGION_PLDS,
	REGION_MAX
};

/* Useful macros to deal with bit shuffling encryptions */
#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP32(val,B31,B30,B29,B28,B27,B26,B25,B24,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B31) << 31) | \
		 (BIT(val,B30) << 30) | \
		 (BIT(val,B29) << 29) | \
		 (BIT(val,B28) << 28) | \
		 (BIT(val,B27) << 27) | \
		 (BIT(val,B26) << 26) | \
		 (BIT(val,B25) << 25) | \
		 (BIT(val,B24) << 24) | \
		 (BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
#define BYTE_XOR_LE(a)  	(a)
#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
#define BYTE4_XOR_LE(a) 	(a)
#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
#define WORD_XOR_LE(a)  	(a)
#define BYTE8_XOR_BE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
#define BYTE8_XOR_LE(a) 	(a)
#define WORD2_XOR_BE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
#define WORD2_XOR_LE(a)  	(a)
#define DWORD_XOR_BE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
#define DWORD_XOR_LE(a)  	(a)


#endif
