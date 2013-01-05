#ifndef _NEO_IO_H
#define _NEO_IO_H

#include "NeoSystem.h"

/*#define NEOIO_DEFINE_READ8(name) \
	static inline u8 name##8(u32 a) { \
		if((a & 0x01) == 0) return (u8)(name##16(a) >> 8); \
		else return (u8)name##16(a); \
	}

#define NEOIO_DEFINE_READ32(name) \
	static inline u32 name##32(u32 a) { \
		return ((u32)name##16(a) << 16) | (u32)name##16(a + 2); \
	}

#define NEOIO_DEFINE_WRITE8(name) \
	static inline void name##8(u32 a, u8 d) { \
		name##16(a, (u16)d); \
	}

#define NEOIO_DEFINE_WRITE32(name) \
	static inline void name##32(u32 a, u32 d) { \
		name##16(a, (u16)(d >> 16)); \
		name##16(a + 2, (u16)d); \
	}

#define NEOIO_DEFINE_READ(name) \
	u16 name##16(u32 a); \
	NEOIO_DEFINE_READ8(name) \
	NEOIO_DEFINE_READ32(name)

#define NEOIO_DEFINE_WRITE(name) \
	void name##16(u32 a, u16 d); \
	NEOIO_DEFINE_WRITE8(name) \
	NEOIO_DEFINE_WRITE32(name)*/

u8 neoReadVideo8(u32 a);
u16 neoReadVideo16(u32 a);
u32 neoReadVideo32(u32 a);
/*static inline u8 neoReadVideo8(u32 a)
{
	if((a & 0x01) == 0) {
		//return MSB
		return (u8)(neoReadVideo16(a) >> 8);
	}
	return cpuUnmapped8();
}
NEOIO_DEFINE_READ32(neoReadVideo);*/

u16 neoReadCtrl116(u32 a);
u16 neoReadCtrl216(u32 a);
u16 neoReadCtrl316(u32 a);
u16 neoReadCoin16(u32 a);

//NEOIO_DEFINE_READ(neoReadCtrl1);
//NEOIO_DEFINE_READ(neoReadCtrl2);
//NEOIO_DEFINE_READ(neoReadCtrl3);
//NEOIO_DEFINE_READ(neoReadCoin);

void neoWriteSystemLatch16(u32 a, u16 d);
void neoWrite4990a16(u32 a, u16 d);

//NEOIO_DEFINE_WRITE(neoWriteSystemLatch);
//NEOIO_DEFINE_WRITE(neoWrite4990a);

void neoWriteVideo8(u32 a, u8 d);
void neoWriteVideo16(u32 a, u16 d);
void neoWriteVideo32(u32 a, u32 d);
//static inline void neoWriteVideo8(u32 a, u8 d)
//{
//	if((a & 0x01) == 0) {
//		//even address store same data to both locations
//		neoWriteVideo16(a, (u16)d | ((u16)d << 8));
//	}
//}
//NEOIO_DEFINE_WRITE32(neoWriteVideo);

void neoWriteWatchdog8(u32 a, u8 d);
void neoWriteWatchdog16(u32 a, u16 d);

void neoWriteAudioCommand8(u32 a, u8 d);
void neoWriteAudioCommand16(u32 a, u16 d);
//static inline void neoWriteWatchdog8(u32 a, u8 d)
//{
//	if(a & 0x01) { //only odd byte writes reset watchdog
//		g_neo->watchdogCounter = 0;
//	}
//}
//NEOIO_DEFINE_WRITE32(neoWriteWatchdog);

void neoIOInit();

#endif
