/*
 *	HT Editor
 *	htxexhead.cc
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

#include <cstring>

#include "formats.h"
#include "htapp.h"
#include "atom.h"
#include "htcoff.h"
#include "htctrl.h"
#include "endianess.h"
#include "hthex.h"
#include "htiobox.h"
#include "htnewexe.h"
#include "htxex.h"
#include "htxexhead.h"
#include "httag.h"
#include "strtools.h"
#include "snprintf.h"

#include "xexstruct.h"

static ht_mask_ptable xexmagic[] = {
	{"magic",						STATICTAG_EDIT_CHAR("00000000")STATICTAG_EDIT_CHAR("00000001")STATICTAG_EDIT_CHAR("00000002")STATICTAG_EDIT_CHAR("00000003")},
	{0, 0}
};

static ht_tag_flags_s xbe_init_flags[] =
{
	{-1, "XBE - initialisation flags"},
	{0,  "[00] Mount Utility Drive"},
	{1,  "[01] Format Utility Drive"},
	{2,  "[02] Limit to 64MB"},
	{3,  "[03] Dont setup harddisk"},
	{0, 0}
};

static ht_mask_ptable xeximageheader[] = {
	{"flags",			STATICTAG_EDIT_DWORD_BE("00000004")},
	{"offset of loader",		STATICTAG_EDIT_DWORD_BE("00000008")},
	{"res",				STATICTAG_EDIT_DWORD_BE("0000000c")},
	{"offset of certificate",	STATICTAG_EDIT_DWORD_BE("00000010")},
	{"number of sections",		STATICTAG_EDIT_DWORD_BE("00000014")},
	{0, 0}
};


static ht_view *htxexheader_init(Bounds *b, File *file, ht_format_group *group)
{
	ht_xex_shared_data *xex_shared=(ht_xex_shared_data *)group->get_shared_data();

	ht_xex_header_viewer *v=new ht_xex_header_viewer();
	v->init(b, DESC_XEX_HEADER, VC_EDIT | VC_SEARCH, file, group);

	ht_mask_sub *s;
	ht_collapsable_sub *cs;
	
	s=new ht_mask_sub();
	s->init(file, 0);
	char info[128];
	ht_snprintf(info, sizeof info, "* XEX header");
	s->add_mask(info);
	v->insertsub(s);

	const bool xex_bigendian = true;
	
	s=new ht_mask_sub();
	s->init(file, 1);
	s->add_staticmask_ptable(xexmagic, 0x0, xex_bigendian);
	
	/* image header */
	s->add_staticmask_ptable(xeximageheader, 0x0, xex_bigendian);
	cs=new ht_collapsable_sub();
	cs->init(file, s, 1, "image header", 1);
	v->insertsub(cs);

#if 0
	/* image header */
	s=new ht_mask_sub();
	s->init(file, 2);
	s->add_staticmask_ptable(xbecertificate, xbe_shared->header.certificate_address-xbe_shared->header.base_address, xbe_bigendian);
	cs=new ht_collapsable_sub();
	cs->init(file, s, 1, "certificate", 1);
	v->insertsub(cs);
	
	/* library versions */
	
	for (uint i=0; i<xbe_shared->header.number_of_library_versions; i++) {
		s=new ht_mask_sub();
		s->init(file, 50+i);

		s->add_staticmask_ptable(xbelibraryversion, xbe_shared->header.library_versions_address-xbe_shared->header.base_address+i*sizeof *xbe_shared->libraries, xbe_bigendian);

		char t[256];
		ht_snprintf(t, sizeof t, "library %d: %-9s %d.%d.%d", i, &xbe_shared->libraries[i].library_name, xbe_shared->libraries[i].major_version, xbe_shared->libraries[i].minor_version, xbe_shared->libraries[i].build_version);

		cs=new ht_collapsable_sub();
		cs->init(file, s, 1, t, 1);
	
		v->insertsub(cs);
	}
	
	/* section headers */
	
	for (uint i=0; i<xbe_shared->sections.number_of_sections; i++) {
		char *name;
//		uint ofs;
	
		s=new ht_mask_sub();
		s->init(file, 100+i);

		s->add_staticmask_ptable(xbesectionheader, xbe_shared->header.section_header_address-xbe_shared->header.base_address+i*sizeof *xbe_shared->sections.sections, xbe_bigendian);

		if (xbe_shared->sections.sections[i].section_name_address) {
		
		    name = (char *)xbe_shared->sections.sections[i].section_name_address;

		} else {
		    name = "<empty>";
		}

		char t[256];
		ht_snprintf(t, sizeof t, "section header %d: %s - rva %08x vsize %08x", i, name, xbe_shared->sections.sections[i].virtual_address, xbe_shared->sections.sections[i].virtual_size);

		cs=new ht_collapsable_sub();
		cs->init(file, s, 1, t, 1);
	
		v->insertsub(cs);
	}
#endif
	return v;

}

format_viewer_if htxexheader_if = {
	htxexheader_init,
	0
};

/*
 *	CLASS ht_xex_header_viewer
 */

void ht_xex_header_viewer::init(Bounds *b, char *desc, int caps, File *file, ht_format_group *group)
{
	ht_uformat_viewer::init(b, desc, caps, file, group);
	VIEW_DEBUG_NAME("ht_xex_header_viewer");
}

int ht_xex_header_viewer::ref_sel(LINE_ID *id)
{
	return 1;
}
