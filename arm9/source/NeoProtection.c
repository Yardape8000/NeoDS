#include "Default.h"
#include "EmuSystem.h"
#include "NeoMemory.h"
#include "NeoSystem.h"

static u16 g_pvcRam[0x1000] ALIGN(32);

static inline void pvc_w8(u32 offset, u8 data)
{
	*(((u8*)g_pvcRam) + offset) = data;
}


static inline u8 pvc_r8(u32 offset)
{
	return *(((u8*)g_pvcRam) + offset);
}

static void neoPvcProt1()
{
	u8 b1, b2;

	b1 = pvc_r8(0x1fe1);
	b2 = pvc_r8(0x1fe0);

	pvc_w8(0x1fe2,(((b2>>0)&0xf)<<1)|((b1>>4)&1));
	pvc_w8(0x1fe3,(((b2>>4)&0xf)<<1)|((b1>>5)&1));
	pvc_w8(0x1fe4,(((b1>>0)&0xf)<<1)|((b1>>6)&1));
	pvc_w8(0x1fe5, (b1>>7));
}

static void neoPvcProt2() // on writes to e8/e9/ea/eb
{
	u8 b1, b2, b3, b4;

	b1 = pvc_r8(0x1fe9);
	b2 = pvc_r8(0x1fe8);
	b3 = pvc_r8(0x1feb);
	b4 = pvc_r8(0x1fea);

	pvc_w8(0x1fec,(b2>>1)|((b1>>1)<<4));
	pvc_w8(0x1fed,(b4>>1)|((b2&1)<<4)|((b1&1)<<5)|((b4&1)<<6)|((b3&1)<<7));
}

static void neoPvcWriteBank()
{
	u32 bankaddress = ((g_pvcRam[0xff8]>>8)|(g_pvcRam[0xff9]<<8));
	*(((u8*)g_pvcRam) + (0x1ff0)) = 0xa0;
	*(((u8*)g_pvcRam) + (0x1ff1)) &= 0xfe;
	*(((u8*)g_pvcRam) + (0x1ff3)) &= 0x7f;
	//neogeo_set_main_cpu_bank_address(bankaddress+0x100000);
	if(bankaddress >= g_neo->romBankCount * MB) {
		systemWriteLine("PVC bank overflow: %06X", bankaddress);
		bankaddress = 0;
	}
	neoSetRomBankAddr(bankaddress);

	//neoSetRomBankAddr((g_pvcRam[0xff8] >> 8) | (g_pvcRam[0xff9] << 8));

	//g_pvcRam[0x1ff0] = 0xa0;
	//g_pvcRam[0x1ff1] &= 0xfe;
	//g_pvcRam[0x1ff3] &= 0x7f;
}

static u8 neoReadPvc8(u32 a)
{
	a &= 0x00ffffff;
	if(a >= 0x2fe000) {
		return CPU_READ8(g_pvcRam, a - 0x2fe000);
	}
	return neoReadBankedRom8(a);
}

static u16 neoReadPvc16(u32 a)
{
	a &= 0x00ffffff;
	if(a >= 0x2fe000) {
		return CPU_READ16(g_pvcRam, a - 0x2fe000);
	}
	return neoReadBankedRom16(a);
}

/*static u32 neoReadPvc32(u32 a)
{
	//split may occur across PVC boundry
	const u32 high = neoReadPvc16(a);
	const u32 low = neoReadPvc16(a + 2);
	return (high << 16) | low;
}*/

static void neoWritePvc8(u32 a, u8 d)
{
	a &= 0x00ffffff;
	if(a >= 0x2fe000) {
		const u32 offset = (a - 0x2fe000) >> 1;
		CPU_WRITE8(g_pvcRam, a - 0x2fe000, d);
		if(offset == 0xff0) neoPvcProt1();
		else if(offset >= 0xff4 && offset <= 0xff5) neoPvcProt2();
		else if(offset >= 0xff8) neoPvcWriteBank();
	} else {
		systemWriteLine("PVC out of range: %06X", a);
	}
}

static void neoWritePvc16(u32 a, u16 d)
{
	a &= 0x00ffffff;
	if(a >= 0x2fe000) {
		const u32 offset = (a - 0x2fe000) >> 1;
		CPU_WRITE16(g_pvcRam, a - 0x2fe000, d);
		if(offset == 0xff0) neoPvcProt1();
		else if(offset >= 0xff4 && offset <= 0xff5) neoPvcProt2();
		else if(offset >= 0xff8) neoPvcWriteBank();
	} else {
		systemWriteLine("PVC out of range: %06X", a);
	}
}

/*static void neoWritePvc32(u32 a, u32 d)
{
	//split may occur across PVC boundry
	neoWritePvc16(a, (u16)(d >> 16));
	neoWritePvc16(a + 2, (u16)d);
}*/

static const u32 g_kof99Bankoffset[64] =
{
	0x000000, 0x100000, 0x200000, 0x300000,
	0x3cc000, 0x4cc000, 0x3f2000, 0x4f2000,
	0x407800, 0x507800, 0x40d000, 0x50d000,
	0x417800, 0x517800, 0x420800, 0x520800,
	0x424800, 0x524800, 0x429000, 0x529000,
	0x42e800, 0x52e800, 0x431800, 0x531800,
	0x54d000, 0x551000, 0x567000, 0x592800,
	0x588800, 0x581800, 0x599800, 0x594800,
	0x598000,	/* rest not used? */
};

static const u32 g_kof99Bankbit[6] = {14, 6, 8, 10, 12, 5};

static const u32 g_mslug3Bankoffset[64] = {
  0x000000, 0x020000, 0x040000, 0x060000, // 00
  0x070000, 0x090000, 0x0b0000, 0x0d0000, // 04
  0x0e0000, 0x0f0000, 0x120000, 0x130000, // 08
  0x140000, 0x150000, 0x180000, 0x190000, // 12
  0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000, // 16
  0x200000, 0x210000, 0x240000, 0x250000, // 20
  0x260000, 0x270000, 0x2a0000, 0x2b0000, // 24
  0x2c0000, 0x2d0000, 0x300000, 0x310000, // 28
  0x320000, 0x330000, 0x360000, 0x370000, // 32
  0x380000, 0x390000, 0x3c0000, 0x3d0000, // 36
  0x400000, 0x410000, 0x440000, 0x450000, // 40
  0x460000, 0x470000, 0x4a0000, 0x4b0000, // 44
  0x4c0000, /* rest not used? */
};

static const u32 g_mslug3Bankbit[6] = {14, 12, 15, 6, 3, 9};

static const u32 g_kof2000Bankoffset[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x3f7800, 0x4f7800, 0x3ff800, 0x4ff800, // 04
	0x407800, 0x507800, 0x40f800, 0x50f800, // 08
	0x416800, 0x516800, 0x41d800, 0x51d800, // 12
	0x424000, 0x524000, 0x523800, 0x623800, // 16
	0x526000, 0x626000, 0x528000, 0x628000, // 20
	0x52a000, 0x62a000, 0x52b800, 0x62b800, // 24
	0x52d000, 0x62d000, 0x52e800, 0x62e800, // 28
	0x618000, 0x619000, 0x61a000, 0x61a800, // 32
};

static const u32 g_kof2000Bankbit[6] = {15, 14, 7, 3, 10, 5};

/* thanks to Razoola and Mr K for the info */
static const u32 g_garouBankoffset[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
	0x2f0000, 0x3f0000, 0x400000, 0x500000, // 08
	0x420000, 0x520000, 0x440000, 0x540000, // 12
	0x498000, 0x598000, 0x4a0000, 0x5a0000, // 16
	0x4a8000, 0x5a8000, 0x4b0000, 0x5b0000, // 20
	0x4b8000, 0x5b8000, 0x4c0000, 0x5c0000, // 24
	0x4c8000, 0x5c8000, 0x4d0000, 0x5d0000, // 28
	0x458000, 0x558000, 0x460000, 0x560000, // 32
	0x468000, 0x568000, 0x470000, 0x570000, // 36
	0x478000, 0x578000, 0x480000, 0x580000, // 40
	0x488000, 0x588000, 0x490000, 0x590000, // 44
	0x5d0000, 0x5d8000, 0x5e0000, 0x5e8000, // 48
	0x5f0000, 0x5f8000, 0x600000, /* rest not used? */
};

static const u32 g_garouBankbit[6] = {5, 9, 7, 6, 14, 12};

static const u32 g_garouoBankoffset[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
	0x2c8000, 0x3c8000, 0x400000, 0x500000, // 08
	0x420000, 0x520000, 0x440000, 0x540000, // 12
	0x598000, 0x698000, 0x5a0000, 0x6a0000, // 16
	0x5a8000, 0x6a8000, 0x5b0000, 0x6b0000, // 20
	0x5b8000, 0x6b8000, 0x5c0000, 0x6c0000, // 24
	0x5c8000, 0x6c8000, 0x5d0000, 0x6d0000, // 28
	0x458000, 0x558000, 0x460000, 0x560000, // 32
	0x468000, 0x568000, 0x470000, 0x570000, // 36
	0x478000, 0x578000, 0x480000, 0x580000, // 40
	0x488000, 0x588000, 0x490000, 0x590000, // 44
	0x5d8000, 0x6d8000, 0x5e0000, 0x6e0000, // 48
	0x5e8000, 0x6e8000, 0x6e8000, 0x000000, // 52
	0x000000, 0x000000, 0x000000, 0x000000, // 56
	0x000000, 0x000000, 0x000000, 0x000000, // 60
};

static const u32 g_garouoBankbit[6] = {4, 8, 14, 2, 11, 13};

static void neoWriteRomBankSma16(u32 a, u16 d)
{
	if(a == g_neo->smaBankAddr) {
		/* unscramble bank number */
		d = (((d >> g_neo->smaBankbit[0]) & 1) << 0) +
			(((d >> g_neo->smaBankbit[1]) & 1) << 1) +
			(((d >> g_neo->smaBankbit[2]) & 1) << 2) +
			(((d >> g_neo->smaBankbit[3]) & 1) << 3) +
			(((d >> g_neo->smaBankbit[4]) & 1) << 4) +
			(((d >> g_neo->smaBankbit[5]) & 1) << 5);

		neoSetRomBankAddr(g_neo->smaBankoffset[d]);
	} else {
		neoWriteRomBank(a, d);
	}
}

static void neoWriteRomBankSma8(u32 a, u8 d)
{
	if(a & 1) neoWriteRomBankSma16((a & ~1), (u16)d);
	else neoWriteRomBankSma16((a & ~1), (u16)d << 8);
}

static u16 neoReadSma16(u32 a)
{
	a &= 0xffffff;
	if(a == 0x2fe446) {
		return 0x9a37; //all sma protection games have this register here
	} else if(a == g_neo->smaAddr0 || a == g_neo->smaAddr1) {
		u16 old = g_neo->smaRand;
		u16 newbit = (
			(g_neo->smaRand >> 2) ^
			(g_neo->smaRand >> 3) ^
			(g_neo->smaRand >> 5) ^
			(g_neo->smaRand >> 6) ^
			(g_neo->smaRand >> 7) ^
			(g_neo->smaRand >>11) ^
			(g_neo->smaRand >>12) ^
			(g_neo->smaRand >>15)) & 1;

		g_neo->smaRand = (u16)(g_neo->smaRand << 1) | newbit;

		//systemWriteLine("SMA: %04X", old);

		return old;
	}
	return neoReadBankedRom16(a);
}

static void neoKof98Write16(u32 a, u16 d)
{
	if((a & 0xffffff) == 0x20aaaa) {
		/* info from razoola */
		u16* mem16 = (u16*)g_neo->pRom0;
		switch(d) {
		case 0x0090:
			systemWriteLine("kof98 0x90");
			mem16[0x100/2] = 0x00c2;
			mem16[0x102/2] = 0x00fd;
			break;
		case 0x00f0:
			systemWriteLine("kof98 0xf0");
			mem16[0x100/2] = 0x4e45;
			mem16[0x102/2] = 0x4f2d;
			break;
		default:
			systemWriteLine("kof98 ??? (%04X)", d);
		}
	} else {
		//nothing else is ever mapped here
		systemWriteLine("kof98 address? (%06X)", a);
	}
}

static void neoKof98Write8(u32 a, u8 d)
{
	if(a & 1) {
		neoKof98Write16(a & ~0x01, (u16)d);
	} else {
		neoKof98Write16(a, (u16)(d << 8));
	}
}

static u32 g_fatFury2Prot;

static u16 neoFatFury2Read16(u32 a)
{
	u16 res = g_fatFury2Prot >> 24;

	switch(a & 0xffffe) {
	case 0x55550:
	case 0xffff0:
	case 0x00000:
	case 0xff000:
	case 0x36000:
	case 0x36008:
		return res;
	case 0x36004:
	case 0x3600c:
		return ((res & 0xf0) >> 4) | ((res & 0x0f) << 4);
	default:
		return 0;
	}
}


static void neoFatFury2Write16(u32 a, u16 d)
{
	switch(a & 0xffffe) {
	case 0x11112: /* data == 0x1111; expects 0xff000000 back */
		g_fatFury2Prot = 0xff000000;
		break;
	case 0x33332: /* data == 0x3333; expects 0x0000ffff back */
		g_fatFury2Prot = 0x0000ffff;
		break;
	case 0x44442: /* data == 0x4444; expects 0x00ff0000 back */
		g_fatFury2Prot = 0x00ff0000;
		break;
	case 0x55552: /* data == 0x5555; read back from 55550, ffff0, 00000, ff000 */
		g_fatFury2Prot = 0xff00ff00;
		break;
	case 0x56782: /* data == 0x1234; read back from 36000 *or* 36004 */
		g_fatFury2Prot = 0xf05a3601;
		break;
	case 0x42812: /* data == 0x1824; read back from 36008 *or* 3600c */
		g_fatFury2Prot = 0x81422418;
		break;
	case 0x55550:
	case 0xffff0:
	case 0xff000:
	case 0x36000:
	case 0x36004:
	case 0x36008:
	case 0x3600c:
		g_fatFury2Prot <<= 8;
		break;
	}
}

void neoInstallProtection()
{
	u32 i;

	g_neo->smaAddr0 = 0;
	g_neo->smaAddr1 = 0;
	g_neo->smaBankAddr = 0;

	switch(g_header.protection) {
	case NEOPROT_PVC:
		g_neo->cpuRead8Table[0x2f] = neoReadPvc8;
		g_neo->cpuRead16Table[0x2f] = neoReadPvc16;
		g_neo->cpuRead32Table[0x2f] = neoDefaultRead32;//neoReadPvc32;
		g_neo->cpuWrite8Table[0x2f] = neoWritePvc8;
		g_neo->cpuWrite16Table[0x2f] = neoWritePvc16;
		g_neo->cpuWrite32Table[0x2f] = neoDefaultWrite32;//neoWritePvc32;
		memset(g_pvcRam, 0, 0x2000);
		systemWriteLine("PVC protection installed");
		break;
	case NEOPROT_KOF2000:
		g_neo->smaAddr0 = 0x2fffd8;
		g_neo->smaAddr1 = 0x2fffda;
		g_neo->smaBankAddr = 0x2fffec;
		g_neo->smaBankoffset = g_kof2000Bankoffset;
		g_neo->smaBankbit = g_kof2000Bankbit;
		break;
	case NEOPROT_MSLUG3:
		g_neo->smaBankAddr = 0x2fffe4;
		g_neo->smaBankoffset = g_mslug3Bankoffset;
		g_neo->smaBankbit = g_mslug3Bankbit;
		//don't have any sma generator, but still need 0x9a37 protection
		g_neo->smaAddr0 = 1;
		break;
	case NEOPROT_GAROUO:
		g_neo->smaAddr0 = 0x2fffcc;
		g_neo->smaAddr1 = 0x2ffff0;
		g_neo->smaBankAddr = 0x2fffc0;
		g_neo->smaBankoffset = g_garouoBankoffset;
		g_neo->smaBankbit = g_garouoBankbit;
		break;
	case NEOPROT_GAROU:
		g_neo->smaAddr0 = 0x2fffcc;
		g_neo->smaAddr1 = 0x2ffff0;
		g_neo->smaBankAddr = 0x2fffc0;
		g_neo->smaBankoffset = g_garouBankoffset;
		g_neo->smaBankbit = g_garouBankbit;
		break;
	case NEOPROT_KOF99:
		g_neo->smaAddr0 = 0x2ffff8;
		g_neo->smaAddr1 = 0x2ffffa;
		g_neo->smaBankAddr = 0x2ffff0;
		g_neo->smaBankoffset = g_kof99Bankoffset;
		g_neo->smaBankbit = g_kof99Bankbit;
		break;
	case NEOPROT_KOF98:
		g_neo->cpuWrite8Table[0x20] = neoKof98Write8;
		g_neo->cpuWrite16Table[0x20] = neoKof98Write16;
		systemWriteLine("KOF98 protection installed");
		break;
	case NEOPROT_FATFURY2:
		for(i = 0x20; i < 0x30; i++) {
			g_neo->cpuWrite8Table[i] = (TWrite8Func)neoFatFury2Write16; //???
			g_neo->cpuWrite16Table[i] = neoFatFury2Write16;
			g_neo->cpuRead16Table[i] = neoFatFury2Read16;
		}
		systemWriteLine("FATFURY2 protection installed");
		g_fatFury2Prot = 0;
		break;
	}

	if(g_neo->smaBankAddr != 0) {
		g_neo->cpuWrite8Table[0x2f] = neoWriteRomBankSma8;
		g_neo->cpuWrite16Table[0x2f] = neoWriteRomBankSma16;
	}

	if(g_neo->smaAddr0 != 0 && g_neo->smaAddr1 != 0) {
		systemWriteLine("SMA installed %06X / %06X",
			g_neo->smaAddr0,
			g_neo->smaAddr1);
		g_neo->cpuRead8Table[0x2f] = neoDefaultRead8;
		g_neo->cpuRead16Table[0x2f] = neoReadSma16;
		g_neo->cpuRead32Table[0x2f] = neoDefaultRead32;
	}
}
