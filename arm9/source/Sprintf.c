/*
* Copyright Patrick Powell 1995
* This code is based on code written by Patrick Powell (papowell@astart.com)
* It may be used for any purpose as long as this notice remains intact
* on all source code distributions
*/

/**************************************************************
* Original:
* Patrick Powell Tue Apr 11 09:48:21 PDT 1995
* A bombproof version of doprnt (dopr) included.
* Sigh.  This sort of thing is always nasty do deal with.  Note that
* the version here does not include floating point...
*
* snprintf() is used instead of sprintf() as it does limit checks
* for string length.  This covers a nasty loophole.
*
* The other functions are there to prevent NULL pointers from
* causing nast effects.
*
* More Recently:
*  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
*  This was ugly.  It is still ugly.  I opted out of floating point
*  numbers, but the formatter understands just about everything
*  from the normal C string format, at least as far as I can tell from
*  the Solaris 2.5 printf(3S) man page.
*
*  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
*    Ok, added some minimal floating point support, which means this
*    probably requires libm on most operating systems.  Don't yet
*    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
*    was pretty badly broken, it just wasn't being exercised in ways
*    which showed it, so that's been fixed.  Also, formated the code
*    to mutt conventions, and removed dead code left over from the
*    original.  Also, there is now a builtin-test, just compile with:
*           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
*    and run snprintf for results.
* 
*  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
*    The PGP code was using unsigned hexadecimal formats. 
*    Unfortunately, unsigned formats simply didn't work.
*
*  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
*    The original code assumed that both snprintf() and vsnprintf() were
*    missing.  Some systems only have snprintf() but not vsnprintf(), so
*    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
*
*  Andrew Tridgell (tridge@samba.org) Oct 1998
*    fixed handling of %.0f
*    added test for HAVE_LONG_DOUBLE
*
*  Russ Allbery <rra@stanford.edu> 2000-08-26
*    fixed return value to comply with C99
*    fixed handling of snprintf(NULL, ...)
*
*  BEN - removed floating point, adapted for NeoDS
*    Smaller by ~8k than even iprintf (probably faster too)
*    vsniprintf was also crashing on me (maybe my fault, maybe not)
*
**************************************************************/

#include "Default.h"
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>

int neoVsnprintf (char *str, size_t count, const char *fmt, va_list arg);

static int dopr (char *buffer, size_t maxlen, const char *format, 
				 va_list args);
static int fmtstr (char *buffer, size_t *currlen, size_t maxlen,
				   char *value, int flags, int min, int max);
static int fmtint (char *buffer, size_t *currlen, size_t maxlen,
				   long value, int base, int min, int max, int flags);
//static int fmtfp (char *buffer, size_t *currlen, size_t maxlen,
//		  LDOUBLE fvalue, int min, int max, int flags);
static int dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c );

/*
* dopr(): poor man's version of doprintf
*/

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2

#define char_to_int(p) (p - '0')
#define MAX(p,q) ((p >= q) ? p : q)
#define MIN(p,q) ((p <= q) ? p : q)

static int dopr (char *buffer, size_t maxlen, const char *format, va_list args)
{
	char ch;
	long value;
	char *strvalue;
	int min;
	int max;
	int state;
	int flags;
	int cflags;
	int total;
	size_t currlen;

	state = DP_S_DEFAULT;
	currlen = flags = cflags = min = 0;
	max = -1;
	ch = *format++;
	total = 0;

	while (state != DP_S_DONE)
	{
		if (ch == '\0')
			state = DP_S_DONE;

		switch(state) 
		{
		case DP_S_DEFAULT:
			if (ch == '%') 
				state = DP_S_FLAGS;
			else 
				total += dopr_outch (buffer, &currlen, maxlen, ch);
			ch = *format++;
			break;
		case DP_S_FLAGS:
			switch (ch) 
			{
			case '-':
				flags |= DP_F_MINUS;
				ch = *format++;
				break;
			case '+':
				flags |= DP_F_PLUS;
				ch = *format++;
				break;
			case ' ':
				flags |= DP_F_SPACE;
				ch = *format++;
				break;
			case '#':
				flags |= DP_F_NUM;
				ch = *format++;
				break;
			case '0':
				flags |= DP_F_ZERO;
				ch = *format++;
				break;
			default:
				state = DP_S_MIN;
				break;
			}
			break;
		case DP_S_MIN:
			if (isdigit(ch)) 
			{
				min = 10*min + char_to_int (ch);
				ch = *format++;
			} 
			else if (ch == '*') 
			{
				min = va_arg (args, int);
				ch = *format++;
				state = DP_S_DOT;
			} 
			else 
				state = DP_S_DOT;
			break;
		case DP_S_DOT:
			if (ch == '.') 
			{
				state = DP_S_MAX;
				ch = *format++;
			} 
			else 
				state = DP_S_MOD;
			break;
		case DP_S_MAX:
			if (isdigit(ch)) 
			{
				if (max < 0)
					max = 0;
				max = 10*max + char_to_int (ch);
				ch = *format++;
			} 
			else if (ch == '*') 
			{
				max = va_arg (args, int);
				ch = *format++;
				state = DP_S_MOD;
			} 
			else 
				state = DP_S_MOD;
			break;
		case DP_S_MOD:
			/* Currently, we don't support Long Long, bummer */
			switch (ch) 
			{
			case 'h':
				cflags = DP_C_SHORT;
				ch = *format++;
				break;
			case 'l':
				cflags = DP_C_LONG;
				ch = *format++;
				break;
			default:
				break;
			}
			state = DP_S_CONV;
			break;
		case DP_S_CONV:
			switch (ch) 
			{
			case 'd':
			case 'i':
				if (cflags == DP_C_SHORT) 
					value = va_arg (args, int);
				else if (cflags == DP_C_LONG)
					value = va_arg (args, long int);
				else
					value = va_arg (args, int);
				total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
				break;
			case 'o':
				flags |= DP_F_UNSIGNED;
				if (cflags == DP_C_SHORT)
					value = va_arg (args, unsigned int);
				else if (cflags == DP_C_LONG)
					value = va_arg (args, unsigned long int);
				else
					value = va_arg (args, unsigned int);
				total += fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
				break;
			case 'u':
				flags |= DP_F_UNSIGNED;
				if (cflags == DP_C_SHORT)
					value = va_arg (args, unsigned int);
				else if (cflags == DP_C_LONG)
					value = va_arg (args, unsigned long int);
				else
					value = va_arg (args, unsigned int);
				total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
				break;
			case 'X':
				flags |= DP_F_UP;
			case 'x':
				flags |= DP_F_UNSIGNED;
				if (cflags == DP_C_SHORT)
					value = va_arg (args, unsigned int);
				else if (cflags == DP_C_LONG)
					value = va_arg (args, unsigned long int);
				else
					value = va_arg (args, unsigned int);
				total += fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
				break;
			case 'c':
				total += dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
				break;
			case 's':
				strvalue = va_arg (args, char *);
				total += fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
				break;
			case 'p':
				strvalue = va_arg (args, void *);
				total += fmtint (buffer, &currlen, maxlen, (long) strvalue, 16, min,
					max, flags);
				break;
			case '%':
				total += dopr_outch (buffer, &currlen, maxlen, ch);
				break;
			case 'w':
				/* not supported yet, treat as next char */
				ch = *format++;
				break;
			default:
				/* Unknown, skip */
				break;
			}
			ch = *format++;
			state = DP_S_DEFAULT;
			flags = cflags = min = 0;
			max = -1;
			break;
		case DP_S_DONE:
			break;
		default:
			/* hmm? */
			break; /* some picky compilers need this */
		}
	}
	if (buffer != NULL)
	{
		if (currlen < maxlen - 1) 
			buffer[currlen] = '\0';
		else 
			buffer[maxlen - 1] = '\0';
	}
	return total;
}

static int fmtstr (char *buffer, size_t *currlen, size_t maxlen,
				   char *value, int flags, int min, int max)
{
	int padlen, strln;     /* amount to pad */
	int cnt = 0;
	int total = 0;

	if (value == 0)
	{
		value = "<NULL>";
	}

	for (strln = 0; value[strln]; ++strln); /* strlen */
	if (max >= 0 && max < strln)
		strln = max;
	padlen = min - strln;
	if (padlen < 0) 
		padlen = 0;
	if (flags & DP_F_MINUS) 
		padlen = -padlen; /* Left Justify */

	while (padlen > 0)
	{
		total += dopr_outch (buffer, currlen, maxlen, ' ');
		--padlen;
	}
	while (*value && ((max < 0) || (cnt < max)))
	{
		total += dopr_outch (buffer, currlen, maxlen, *value++);
		++cnt;
	}
	while (padlen < 0)
	{
		total += dopr_outch (buffer, currlen, maxlen, ' ');
		++padlen;
	}
	return total;
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static int fmtint (char *buffer, size_t *currlen, size_t maxlen,
				   long value, int base, int min, int max, int flags)
{
	int signvalue = 0;
	unsigned long uvalue;
	char convert[32];
	int place = 0;
	int spadlen = 0; /* amount to space pad */
	int zpadlen = 0; /* amount to zero pad */
	int caps = 0;
	int total = 0;

	if (max < 0)
		max = 0;

	uvalue = value;

	if(!(flags & DP_F_UNSIGNED))
	{
		if( value < 0 ) {
			signvalue = '-';
			uvalue = -value;
		}
		else
			if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
				signvalue = '+';
			else
				if (flags & DP_F_SPACE)
					signvalue = ' ';
	}

	if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

	do {
		convert[place++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")
			[uvalue % (unsigned)base  ];
		uvalue = (uvalue / (unsigned)base );
	} while(uvalue && (place < sizeof(convert)));
	if (place == sizeof(convert)) place--;
	convert[place] = 0;

	zpadlen = max - place;
	spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
	if (zpadlen < 0) zpadlen = 0;
	if (spadlen < 0) spadlen = 0;
	if (flags & DP_F_ZERO)
	{
		zpadlen = MAX(zpadlen, spadlen);
		spadlen = 0;
	}
	if (flags & DP_F_MINUS) 
		spadlen = -spadlen; /* Left Justifty */

	/* Spaces */
	while (spadlen > 0) 
	{
		total += dopr_outch (buffer, currlen, maxlen, ' ');
		--spadlen;
	}

	/* Sign */
	if (signvalue) 
		total += dopr_outch (buffer, currlen, maxlen, signvalue);

	/* Zeros */
	if (zpadlen > 0) 
	{
		while (zpadlen > 0)
		{
			total += dopr_outch (buffer, currlen, maxlen, '0');
			--zpadlen;
		}
	}

	/* Digits */
	while (place > 0) 
		total += dopr_outch (buffer, currlen, maxlen, convert[--place]);

	/* Left Justified spaces */
	while (spadlen < 0) {
		total += dopr_outch (buffer, currlen, maxlen, ' ');
		++spadlen;
	}

	return total;
}

static int dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c)
{
	if (*currlen + 1 < maxlen)
		buffer[(*currlen)++] = c;
	return 1;
}

int neoSprintf (char *str, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int count = neoVsnprintf(str, 256, fmt, args);
	va_end(args);
	return count;
}

int neoVsnprintf (char *str, size_t count, const char *fmt, va_list args)
{
	if (str != NULL)
		str[0] = 0;
	return dopr(str, count, fmt, args);
	
	//strncpy(str, fmt, count);
	//return 0;
}

