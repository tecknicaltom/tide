/* 
 *	HT Editor
 *	htmzrel.cc
 *
 *	Copyright (C) 1999, 2000, 2001 Stefan Weyergraf (stefan@weyergraf.de)
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

#include "htmz.h"
#include "htmzrel.h"
#include "htpal.h"
#include "httag.h"
#include "formats.h"

int htmzrel_detect(ht_streamfile *file)
{
	word rcount;
	file->seek(6);
	file->read(&rcount, 2);
	return rcount;
}

ht_view *htmzrel_init(bounds *b, ht_streamfile *file, ht_format_group *group)
{
	ht_uformat_viewer *v=new ht_uformat_viewer();
	v->init(b, DESC_MZ_REL, VC_EDIT | VC_SEARCH, file, group);
	dword r=0;
	dword rc=0;
	file->seek(6);
	file->read(&rc, 2);
	file->seek(24);
	file->read(&r, 2);
	char buf[256];

	ht_mask_sub *m=new ht_mask_sub();
	m->init(file, 0);
	char info[128];
	sprintf(info, "* MZ relocations at offset %08x", r);
	m->add_mask(info);
	for (int i=rc; i>0; i--) {
		int so;
		file->seek(r);
		file->read(&so, 4);
		char *b=tag_make_edit_word(buf, r+2, tag_endian_little);
		*(b++)=':';
		b=tag_make_edit_word(b, r, tag_endian_little);
		*b=0;
		m->add_mask(buf);
		r+=4;
	}
	v->insertsub(m);
	return v;
}

/*viewer_int htmzrel_int = {
	htmzrel_detect,
	htmzrel_init
};*/
