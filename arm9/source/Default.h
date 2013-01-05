#ifndef _DEFAULT_H
#define _DEFAULT_H

#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct _TPoint {
	s32 x;
	s32 y;
} TPoint;

typedef union _TBounds {
	struct {
		s32 x0;
		s32 y0;
		s32 x1;
		s32 y1;
	};
	struct {
		TPoint p0;
		TPoint p1;
	};
} TBounds;

static inline bool boundsTest(const TBounds* pBounds, s32 x, s32 y)
{
	return x >= pBounds->x0 && y >= pBounds->y0 &&
		x <= pBounds->x1 && y <= pBounds->y1;
}

#define ATTR_WEAK __attribute__ ((weak))
#define ATTR_UNUSED __attribute__ ((unused))

#include "EmuSystem.h"
#include "NeoSystem.h"

int neoSprintf (char *str, const char *fmt, ...);
int neoVsnprintf (char *str, size_t count, const char *fmt, va_list args);

#endif
