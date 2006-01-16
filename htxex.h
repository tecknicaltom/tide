/* 
 *	HT Editor
 *	htxex.h
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

#ifndef __HTXEX_H__
#define __HTXEX_H__

#include "formats.h"
#include "xexstruct.h"

#define DESC_XEX "xex - xenon executable"
#define DESC_XEX_HEADER "xex/header"


extern format_viewer_if htxex_if;

struct xex_info_entry {
	FileOfs start;
	FileOfs size;
	uint32 type;
};

struct ht_xex_shared_data {
	XEX_IMAGE_HEADER header;
	XEX_IMAGE_HEADER_INFO_ENTRY *info_table;
	xex_info_entry *info_table_cooked;

	uint32 certificate_offset;
	uint32 certificate_size;
	uint32 original_base_address;
	uint32 entry_point;
	uint32 load_address;

	ht_format_viewer *v_header;
};

/*
 *	ht_xex
 */
class ht_xex: public ht_format_group {
protected:
	bool loc_enum;
public:
		void init(Bounds *b, File *file, format_viewer_if **ifs, ht_format_group *format_group, FileOfs header_ofs);
	virtual	void done();
};

#endif /* !__HTXEX_H__ */
