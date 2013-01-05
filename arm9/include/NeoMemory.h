#ifndef _NEO_MEMORY_H
#define _NEO_MEMORY_H

#define BANK_MAX 8 //power of 2

#define BANK_CACHE_SHIFT 12
#define BANK_CACHE_ENTRY_SIZE (1 << BANK_CACHE_SHIFT)
//#define BANK_CACHE_SIZE (512*KB)
//#define BANK_CACHE_COUNT (BANK_CACHE_SIZE / BANK_CACHE_ENTRY_SIZE)
#define BANK_TABLE_SIZE (8*MB / BANK_CACHE_ENTRY_SIZE)

#define BANK_CACHE_INVALID_ASM 0xff000000
#define BANK_CACHE_INVALID ((u8*)BANK_CACHE_INVALID_ASM)

#ifndef NEO_IN_ASM
//extern u8 g_bankCache[];
//extern u8 g_mainProgram[1*MB];
//extern u8 g_bios[128*KB];
//extern u16 g_vram[0x8000];
//extern u16 g_spriteMem[2*KB];
//extern u8 g_fixedRom[128*KB];
extern u16 g_paletteMem[8*KB];
//extern u8 g_sram[64*KB];
//extern u8 g_ram[64*KB];

u16 neoReadCard16(u32 a);
void neoWriteCard8(u32 a, u8 d);
void neoWriteCard16(u32 a, u16 d);

void neoWriteSram8(u32 a, u8 d);
void neoWriteSram16(u32 a, u16 d);
//void neoWriteSram32(u32 a, u32 d);

u8 neoReadSram8(u32 a);
u16 neoReadSram16(u32 a);
u32 neoReadSram32(u32 a);
//static inline void neoWriteSram32(u32 a, u32 d)
//{
//	neoWriteSram16(a, (u16)(d >> 16));
//	neoWriteSram16(a + 2, (u16)d);
//}
void neoSetRomBankAddr(u32 bankAddr);
void neoWriteRomBank(u32 a, u16 d);
u8 neoReadBankedRom8(u32 a);
u16 neoReadBankedRom16(u32 a);
u32 neoReadBankedRom32(u32 a);
u32 neoBankPC(u32 a);

extern u8 neoReadRom8(u32 a);
extern u16 neoReadRom16(u32 a);
extern u32 neoReadRom32(u32 a);

extern u8 neoReadSlot2Rom8(u32 a);
extern u16 neoReadSlot2Rom16(u32 a);
extern u32 neoReadSlot2Rom32(u32 a);

extern u8 neoReadBios8(u32 a);
extern u16 neoReadBios16(u32 a);
extern u32 neoReadBios32(u32 a);

//unbanked read from 2nd bank
extern u8 neoReadRom18(u32 a);
extern u16 neoReadRom116(u32 a);
extern u32 neoReadRom132(u32 a);

//first bank read from 2nd bank
extern u8 neoReadRom08(u32 a);
extern u16 neoReadRom016(u32 a);
extern u32 neoReadRom032(u32 a);

extern u8 neoReadRam8(u32 a);
extern u16 neoReadRam16(u32 a);
extern u32 neoReadRam32(u32 a);

extern void neoWriteRam8(u32 a, u8 d);
extern void neoWriteRam16(u32 a, u16 d);
extern void neoWriteRam32(u32 a, u32 d);

extern u32 neoRomPc(u32 pc);
extern u32 neoSlot2RomPc(u32 pc);
extern u32 neoRom0Pc(u32 pc);
extern u32 neoRom1Pc(u32 pc);
extern u32 neoRamPc(u32 pc);
extern u32 neoBiosPc(u32 pc);

u8 neoReadBankedRom8Uncached(u32 a);
u16 neoReadBankedRom16Uncached(u32 a);
u32 neoReadBankedRom32Uncached(u32 a);
u32 neoBankedPcUncached(u32 a);

#define CPU_READ8(buffer, addr) (*((u8*)(buffer) + ((addr) ^ 1)))
#define CPU_READ16(buffer, addr) (*(u16*)((u8*)(buffer) + (addr)))
#define CPU_READ32(buffer, addr) ((u32)(((u32)CPU_READ16((buffer), addr) << 16) | (u32)CPU_READ16((buffer), (addr) + 2)))

#define CPU_WRITE8(buffer, addr, data) *((u8*)(buffer) + ((addr) ^ 1)) = (u8)(data)
#define CPU_WRITE16(buffer, addr, data) *(u16*)((u8*)(buffer) + (addr)) = (u16)(data)
#define CPU_WRITE32(buffer, addr, data) CPU_WRITE16((buffer), addr, (u16)((data) >> 16)); CPU_WRITE16((buffer), (addr) + 2, (u16)(data))

void neoMemoryLoadProgramVector();
void neoMemoryLoadBiosVector();
void neoMemoryInit();

#endif //NEO_IN_ASM

#endif
