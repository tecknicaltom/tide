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
#include "stream.h"
#include "tools.h"

static format_viewer_if *htxex_ifs[] = {
	&htxexheader_if,
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

	/* read header */
	file->seek(0);
	file->read(&xex_shared->header, sizeof xex_shared->header);
	createHostStruct(&xex_shared->header, XEX_IMAGE_HEADER_struct, big_endian);	

	ht_format_group::init_ifs(ifs);
}

void ht_xex::done()
{
	ht_format_group::done();

/*	ht_xbe_shared_data *xbe_shared = (ht_xbe_shared_data*)shared_data;

	free(xbe_shared->sections.sections);

	free(shared_data);*/
}

