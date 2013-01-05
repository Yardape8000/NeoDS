#ifndef _NEO_SYSTEM_COMMON_H
#define _NEO_SYSTEM_COMMON_H

#define RoundUp4B(x) (((u32)(x) & ~3) + 4)
#define RoundUp2B(x) (((u32)(x) & ~1) + 2)

#define MAIN_CLOCK (24*1000*1000)

#define PIXEL_CLOCK_DIVIDE 4

#define PIXELS_PER_SCANLINE 384
#define SCANLINES_PER_FRAME 264
#define SCANLINES_PER_VBLANK 8
#define SCANLINES_TOP 16
#define SCANLINES_BOTTOM 16
#define SCANLINES_ACTIVE 224
#define PIXELS_PER_FRAME (PIXELS_PER_SCANLINE * SCANLINES_PER_FRAME)

#define TOKEN_PASTE2(a, b) TOKEN_PASTE2_INTERNAL(a, b)
#define TOKEN_PASTE2_INTERNAL(a, b) a ## b

#define TOKEN_PASTE3(a, b, c) TOKEN_PASTE3_INTERNAL(a, b, c)
#define TOKEN_PASTE3_INTERNAL(a, b, c) a ## b ## c

#define STATIC_ASSERT(x) \
struct TOKEN_PASTE3(__STATICASSERTSTRUCT, __LINE__, __) { \
	u8 staticAssert[(x) - 1]; \
}

#endif
