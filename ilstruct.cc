/* 
 *	HT Editor
 *	ilstruct.cc
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


#include "htendian.h"
#include "ilstruct.h"

byte IL_METADATA_SECTION_struct[] = {
	STRUCT_ENDIAN_DWORD | STRUCT_ENDIAN_HOST,
	STRUCT_ENDIAN_WORD | STRUCT_ENDIAN_HOST,
	STRUCT_ENDIAN_WORD | STRUCT_ENDIAN_HOST,
	0
};

byte IL_METADATA_SECTION_ENTRY_struct[] = {
	STRUCT_ENDIAN_DWORD | STRUCT_ENDIAN_HOST,
	STRUCT_ENDIAN_DWORD | STRUCT_ENDIAN_HOST,
	0
};
