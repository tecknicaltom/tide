asfas
/* 
 *	HT Editor
 *	htendian.h
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

#ifndef __HTENDIAN_H__
#define __HTENDIAN_H__

#include "io/types.h"

#define STRUCT_ENDIAN_BYTE  1
#define STRUCT_ENDIAN_WORD  2
#define STRUCT_ENDIAN_DWORD 4
#define STRUCT_ENDIAN_QWORD 8
#define STRUCT_ENDIAN_HOST  128

enum endianess {big_endian, little_endian};
void create_foreign_int(void *buf, int i, int size, endianess to_endianess);
void create_foreign_int64(void *buf, const uint64 i, int size, endianess to_endianess);
int create_host_int(const void *buf, int size, endianess from_endianess);
uint64 create_host_int64(const void *buf, endianess from_endianess);
void create_host_struct(void *buf, const byte *table, endianess from);

#endif /* __HTENDIAN_H__ */

