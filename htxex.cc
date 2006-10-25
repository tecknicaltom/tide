/*
 *	HT Editor
 *	htxex.cc
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

#include <cstdlib>
#include <cstring>

#include "log.h"
#include "endianess.h"
#include "htxex.h"
#include "htxexhead.h"
#include "htxeximg.h"
#include "stream.h"
#include "tools.h"

static format_viewer_if *htxex_ifs[] = {
	&htxexheader_if,
	&htxeximage_if,
	0
};

static ht_view *htxex_init(Bounds *b, File *file, ht_format_group *format_group)
{
	byte xexmagic[4];

	file->seek(0);
	file->read(xexmagic, 4);
	if (xexmagic[0]!=XEX_MAGIC0 || xexmagic[1]!=XEX_MAGIC1 ||
	    xexmagic[2]!=XEX_MAGIC2 || xexmagic[3]!=XEX_MAGIC3) return 0;

	ht_xex *g=new ht_xex();
	g->init(b, file, htxex_ifs, format_group, 0);
	return g;
}

format_viewer_if htxex_if = {
	htxex_init,
	0
};

/*
 *	CLASS ht_xex
 */
void ht_xex::init(Bounds *b, File *file, format_viewer_if **ifs, ht_format_group *format_group, FileOfs header_ofs)
{
	ht_format_group::init(b, VO_BROWSABLE | VO_SELECTABLE | VO_RESIZE, DESC_XEX, file, false, true, 0, format_group);
	VIEW_DEBUG_NAME("ht_xex");

	ht_xex_shared_data *xex_shared = ht_malloc(sizeof (ht_xex_shared_data));
	shared_data = xex_shared;

	xex_shared->v_header = NULL;
	xex_shared->image = NULL;
	xex_shared->loader_info.type = XEX_LOADER_NONE;

	/* read header */
	file->seek(0);
	file->read(&xex_shared->header, sizeof xex_shared->header);
	createHostStruct(&xex_shared->header, XEX_IMAGE_HEADER_struct, big_endian);
	xex_shared->info_table = ht_malloc(xex_shared->header.number_of_sections * sizeof *xex_shared->info_table);
	xex_shared->info_table_cooked = ht_malloc(xex_shared->header.number_of_sections * sizeof *xex_shared->info_table_cooked);
	for (uint i=0; i < xex_shared->header.number_of_sections; i++) {
		file->read(xex_shared->info_table+i, sizeof *xex_shared->info_table);
		createHostStruct(xex_shared->info_table+i, XEX_IMAGE_HEADER_INFO_ENTRY_struct, big_endian);
		xex_shared->info_table_cooked[i].type = createHostInt(&xex_shared->info_table[i].type, 4, big_endian);
		xex_shared->info_table_cooked[i].start = 0;
		xex_shared->info_table_cooked[i].size = xex_shared->info_table[i].b.size;
		if (xex_shared->info_table[i].b.size == 0xff) {
			FileOfs ofs = file->tell();
			file->seek(xex_shared->info_table[i].value);
			uint32 s;
			if (file->read(&s, 4) == 4) {
				xex_shared->info_table_cooked[i].start = xex_shared->info_table[i].value + 4;
				xex_shared->info_table_cooked[i].size = createHostInt(&s, 4, big_endian) - 4;
			}
			file->seek(ofs);
		} else if (xex_shared->info_table[i].b.size > 1) {
			xex_shared->info_table_cooked[i].start = xex_shared->info_table[i].value;
			xex_shared->info_table_cooked[i].size = xex_shared->info_table[i].b.size * 4;
		}
	}

	for (uint i=0; i < xex_shared->header.number_of_sections; i++) {
		switch (xex_shared->info_table_cooked[i].type) {
		case XEX_HEADER_FIELD_LOADERINFO: {
			uint32 size = xex_shared->info_table_cooked[i].size;
			if (size > 4) {
				XEX_LOADER_INFO_HEADER ldhdr;
				file->seek(xex_shared->info_table_cooked[i].start);
				if (file->read(&ldhdr, sizeof ldhdr) == sizeof ldhdr) {
					size -= 4;
					createHostStruct(&ldhdr, XEX_LOADER_INFO_HEADER_struct, big_endian);
					if (!ldhdr.crypted && ldhdr.type == XEX_LOADER_RAW) {
						int entries = size / 8;
						xex_shared->loader_info.type = XEX_LOADER_RAW;
						xex_shared->loader_info.raw.sections = new Array(true, entries);
						for (int j=0; j < entries; j++) {
							XEX_RAW_LOADER_ENTRY e;
							if (file->read(&e, sizeof e) != sizeof e) break;
							createHostStruct(&e, XEX_RAW_LOADER_ENTRY_struct, big_endian);
							xex_shared->loader_info.raw.sections->insert(
								new XEXLoaderRawSection(e.raw, e.pad));
						}
					}
				}
			}
			break;
		}
		case XEX_HEADER_FIELD_IMPORT:
			break;
		case XEX_HEADER_FIELD_ENTRY:
			xex_shared->entrypoint = xex_shared->info_table[i].value;
			break;
		case XEX_HEADER_FIELD_BASE:
			xex_shared->image_base = xex_shared->info_table[i].value;
			break;
		}
	}

	file->seek(xex_shared->header.file_header_offset);
	if (file->read(&xex_shared->file_header, sizeof xex_shared->file_header) == sizeof xex_shared->file_header) {
		createHostStruct(&xex_shared->file_header, XEX_FILE_HEADER_struct, big_endian);
		xex_shared->image_base = xex_shared->file_header.load_address;
		xex_shared->image_size = xex_shared->file_header.image_size;
	} else {
		xex_shared->file_header.hdr_size = 0;
		xex_shared->image_size = 0;
		xex_shared->image_base = 0;
	}
	
	xex_shared->image = new MemoryFile(0, xex_shared->image_size);

	try {
		if (xex_shared->loader_info.type == XEX_LOADER_RAW) {
			file->seek(xex_shared->header.size);
			FileOfs ofs = 0;
			foreach(XEXLoaderRawSection, section, *xex_shared->loader_info.raw.sections, {
				xex_shared->image->seek(ofs);
				file->copyTo(xex_shared->image, section->raw);
				ofs += section->raw + section->pad;
			});
		}
	} catch (...) {}

	ht_format_group::init_ifs(ifs);
}

void ht_xex::done()
{
	ht_format_group::done();

	ht_xex_shared_data *xex_shared = (ht_xex_shared_data*)shared_data;

	free(xex_shared->info_table);
	free(xex_shared->info_table_cooked);
	free(xex_shared->image);
	free(xex_shared);
}

bool xex_ofs_to_rva(ht_xex_shared_data *xex_shared, FileOfs ofs, RVA &rva)
{
#if 0
	FileOfs xex_ofs = 0;
	RVA xex_va = 0;
	for (int i = 0; i < g_entry_count; i++) {
		if (xex_ofs <= ofs && xex_ofs+g_entries[i].raw > ofs) {
			res = xex_va + (ofs - xex_ofs);
			return true;
		}
		xex_va += g_entries[i].raw + g_entries[i].pad;
		xex_ofs += g_entries[i].raw;
		if (xex_ofs > ofs) return false;
	}
	return false;
#else
	if (ofs < xex_shared->image_size) {
		rva = ofs;
		return true;
	} else {
		return false;
	}
#endif
}

bool xex_rva_to_ofs(ht_xex_shared_data *xex_shared, RVA rva, FileOfs &ofs)
{
#if 0
	FileOfs xex_ofs = 0;
	RVA xex_va = 0;
	for (int i = 0; i < g_entry_count; i++) {
		if (xex_va <= va && xex_va + g_entries[i].read > va) {
			res = xex_ofs + (va - xex_va);
			return true;
		}
		xex_va += g_entries[i].read + g_entries[i].pad;
		xex_ofs += g_entries[i].read;
		if (xex_va > va) return false;
	}
	return false;
#else
	if (rva < xex_shared->image_size) {
		ofs = rva;
		return true;
	} else {
		int a=1;
		fprintf(stderr, "****** %08x %08x", rva, xex_shared->image_size);
		return false;
	}
#endif
}
