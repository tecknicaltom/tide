/* 
 *	HT Editor
 *	htcoff.h
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf (stefan@weyergraf.de)
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

#ifndef __HTCOFF_H__
#define __HTCOFF_H__

#include "formats.h"
#include "coff_s.h"
#include "htcoffhd.h"
#include "htendian.h"

#define DESC_COFF			"coff - unix common obj file"
#define DESC_COFF_HEADER		"coff/header"
#define DESC_COFF_IMAGE			"coff/image"

#define ATOM_COFF_MACHINES 			0xf0450000
#define ATOM_COFF_MACHINES_STR			"f0450000"

#define ATOM_COFF_OPTIONAL_MAGICS 		0xf0450001
#define ATOM_COFF_OPTIONAL_MAGICS_STR 		 "f0450001"

#define ATOM_COFF_CHARACTERISTICS		0xf0450003
#define ATOM_COFF_CHARACTERISTICS_STR		 "f0450003"

#define ATOM_COFF_SECTION_CHARACTERISTICS	0xf0450004
#define ATOM_COFF_SECTION_CHARACTERISTICS_STR	 "f0450004"

extern format_viewer_if htcoff_if;

struct coff_section_headers {
	UINT base_ofs;
	UINT section_count;
	COFF_SECTION_HEADER *sections;
};

struct ht_coff_shared_data {
	dword hdr_ofs;
	COFF_HEADER coffheader;
	word opt_magic;
	endianess endian;
	union {
		COFF_OPTIONAL_HEADER32 coff32header;
	};
	coff_section_headers sections;
	ht_viewer *v_header;
	ht_viewer *v_image;
};

/*
 *	CLASS ht_coff
 */

class ht_coff: public ht_format_group {
private:
	ht_coff_shared_data *coff_shared;
public:
		void init(bounds *b, ht_streamfile *file, format_viewer_if **ifs, ht_format_group *format_group, FILEOFS header_ofs, endianess end);
	virtual	void done();
};

int coff_rva_to_section(coff_section_headers *section_headers, RVA rva, int *section);
int coff_rva_to_ofs(coff_section_headers *section_headers, RVA rva, dword *ofs);
int coff_rva_is_valid(coff_section_headers *section_headers, RVA rva);
int coff_rva_is_physical(coff_section_headers *section_headers, RVA rva);

int coff_ofs_to_rva(coff_section_headers *section_headers, dword ofs, RVA *rva);
int coff_ofs_to_section(coff_section_headers *section_headers, dword ofs, UINT *section);
int coff_ofs_to_rva_and_section(coff_section_headers *section_headers, dword ofs, RVA *rva, UINT *section);

#endif /* !__HTPE_H__ */
