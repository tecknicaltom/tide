/*
 *	HT Editor
 *	htdebug.cc
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "htdebug.h"

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void ht_assert_failed(const char *file, int line, const char *assertion)
{
	fprintf(stderr, "in file %s, line %d: assertion failed: %s\n", file, line, assertion);
#ifndef WIN32
#if 1
	fprintf(stderr, "sending SIGTRAP...");
	raise(SIGTRAP);
#endif
#endif
	exit(1);
}

void ht_error(const char *file, int line, const char *format,...)
{
	va_list arg;
	va_start(arg, format);
	fprintf(stderr, "in file %s, line %d: error: ", file, line);
	vfprintf(stderr, format, arg);
	fputc('\n', stderr);
	va_end(arg);
	exit(1);
}

void ht_trace(char *file, int line, char *format,...)
{
	FILE *ht_debug_file = stderr;
	va_list arg;
	va_start(arg, format);
	fprintf(ht_debug_file, "TRACE: %s.%d: ", file, line);
	vfprintf(ht_debug_file, format, arg);
	fputc('\n', ht_debug_file);
	va_end(arg);
}

void ht_warn(char *file, int line, char *format,...)
{
	va_list arg;
	va_start(arg, format);
	fprintf(stderr, "in file %s, line %d: warning: ", file, line);
	vfprintf(stderr, format, arg);
	fputc('\n', stderr);
	va_end(arg);
}
