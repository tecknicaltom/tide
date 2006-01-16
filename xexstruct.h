/* 
 *	HT Editor
 *	xexstruct.h
 *
 *	Copyright (C) 2006 Sebastian Biallas (sb@biallas.net)
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

#ifndef __XEXSTRUCT_H_
#define __XEXSTRUCT_H_

#include "io/types.h"
#include "tools.h"

typedef unsigned int RVA;

#define XEX_MAGIC_LENGTH 4
#define XEX_MAGIC0	'X'
#define XEX_MAGIC1	'E'
#define XEX_MAGIC2	'X'
#define XEX_MAGIC3	'2'

typedef struct XEX_IMAGE_HEADER {
	byte	magic_id[XEX_MAGIC_LENGTH] PACKED;
	uint32	flags PACKED;
	uint32	offset_unpack PACKED;
	uint32	res PACKED;
	uint32	certificate_address PACKED;
	uint32	number_of_sections PACKED;
};

#define XEX_HEADER_FIELD_MODULES	0x0002ff
#define XEX_HEADER_FIELD_FILEINFO	0x0003ff
#define XEX_HEADER_FIELD_FILENAME	0x0080ff
#define XEX_HEADER_FIELD_LOADBASE	0x010001
#define XEX_HEADER_FIELD_ENTRY		0x010100
#define XEX_HEADER_FIELD_BASE		0x010201
#define XEX_HEADER_FIELD_IMPORT		0x0103ff
#define XEX_HEADER_FIELD_IDS		0x018002
#define XEX_HEADER_FIELD_UPDATE		0x0183ff
#define XEX_HEADER_FIELD_RESMAP2	0x0200ff
#define XEX_HEADER_FIELD_UNK0		0x020104 // 80078884
#define XEX_HEADER_FIELD_UNK1		0x020200 // 800788bc
#define XEX_HEADER_FIELD_CACHE_INFO	0x020301
#define XEX_HEADER_FIELD_MEDIAINFO	0x040006
#define XEX_HEADER_FIELD_UNK2		0x040404
#define XEX_HEADER_FIELD_IMPORT_UNK	0xe10402

typedef struct XEX_IMAGE_HEADER_INFO_ENTRY {
	union {
		struct {
			byte	res PACKED;
			byte	classe PACKED;
			byte	type PACKED;
			byte	size PACKED;
		} b;
		uint32 type PACKED;
	};
	uint32	value PACKED;
};

extern byte XEX_IMAGE_HEADER_struct[];
extern byte XEX_IMAGE_HEADER_INFO_ENTRY_struct[];

#endif
