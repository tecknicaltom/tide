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

typedef struct XEX_IMAGE_HEADER_INFO_ENTRY {
	byte	res PACKED;
	byte	classe PACKED;
	byte	type PACKED;
	byte	size PACKED;
	uint32	value;
};

extern byte XEX_IMAGE_HEADER_struct[];
extern byte XEX_IMAGE_HEADER_INFO_ENTRY_struct[];

#endif
