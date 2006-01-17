/* 
 *	HT Editor
 *	hthex.cc
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

#include <string.h>

#include "cmds.h"
#include "htctrl.h"
#include "htiobox.h"
#include "hthex.h"
#include "htmenu.h"
#include "htsearch.h"
#include "snprintf.h"
#include "stream.h"
#include "tools.h"

extern "C" {
#include "evalx.h"
#include "regex.h"
}

ht_view *hthex_init(Bounds *b, File *file, ht_format_group *group)
{
	ht_hex_viewer *v = new ht_hex_viewer();
	v->init(b, DESC_HEX, VC_EDIT | VC_GOTO | VC_SEARCH | VC_REPLACE | VC_RESIZE, file, group);

	v->search_caps |= SEARCHMODE_BIN | SEARCHMODE_EVALSTR | SEARCHMODE_EXPR;

	v->h = new ht_hex_file_sub();
	v->h->init(file, 0x0, file->getSize(), 16, 0);

	v->insertsub(v->h);
	return v;
}

format_viewer_if hthex_if = {
	hthex_init,
	0
};

/*
 *	CLASS ht_hex_viewer
 */

int ht_hex_viewer::get_pindicator_str(char *buf, int max_len)
{
	FileOfs o;
	if (get_current_offset(&o)) {
		FileOfs sel_start, sel_end;
		pselect_get(&sel_start, &sel_end);
		char ttemp[1024];
		if (sel_end-sel_start > 0) {
			ht_snprintf(ttemp, sizeof ttemp, "selection %qxh-%qxh (%qd byte%s) ", sel_start, sel_end-1, sel_end-sel_start, sel_end-sel_start==1?"":"s");
		} else {
			ttemp[0] = 0;
		}
		return ht_snprintf(buf, max_len, " %s %qxh/%qu %s", edit() ? "edit" : "view", o, o, ttemp);
	} else {
		return ht_snprintf(buf, max_len, " ? ");
	}
}
	
bool ht_hex_viewer::get_vscrollbar_pos(int *pstart, int *psize)
{
	FileOfs s = file->getSize();
	if (s) {
		uint ll = h->get_line_length();
		// FIXPORT
		FileOfs o = top.line_id.id2 - (uint64(top.line_id.id1) << 32);
		int z = MIN(size.h * ll, s - o);
		return scrollbar_pos(o, z, s, pstart, psize);
	}
	return false;
}

void ht_hex_viewer::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
	case msg_filesize_changed:
		htmsg m;
		m.msg=msg_filesize_changed;
		m.type=mt_broadcast;
		sendsubmsg(&m);

		uf_initialized=false;
		complete_init();

		dirtyview();
		return;
	case cmd_hex_entropy: {
		FileOfs ofs;
		if (get_current_offset(&ofs)) {
			byte buf[64];
			if (pread(ofs, buf, 64)==64) {
				int e = calc_entropy2(buf, 64);
				infobox("64-byte entropy at offset %q08x: %d %%", ofs, e);
			}
		}
		clearmsg(msg);
		return;
	}
	case cmd_hex_display_bytes: {
		char result[256];
		sprintf(result, "%d", h->get_line_length());
		if (inputbox("Change display width", "~Line length (Bytes)", result, 256)) {
			char *p;
			int ll = strtoul(result, &p, 10);
			if (ll > 0 && ll <= 32) {
				h->set_line_length(ll);
			} else {
				errorbox("Line length must be > 0 and <= 32!");
			}
		}
		clearmsg(msg);
		return;
	}
	case msg_contextmenuquery: {
		ht_static_context_menu *m = new ht_static_context_menu();
		m->init("~Local-Hex");
		m->insert_entry("~Block operations", "Ctrl+B", cmd_file_blockop, K_Control_B, 1);
		m->insert_entry("~Replace", "Ctrl+E", cmd_file_replace, K_Control_E, 1);
		m->insert_entry("~Entropy", "Ctrl+T", cmd_hex_entropy, K_Control_T, 1);
		m->insert_entry("~Change display width...", "Ctrl+O", cmd_hex_display_bytes, K_Control_O, 1);

		msg->msg = msg_retval;
		msg->data1.ptr = m;
		return;
	}
	}
	ht_uformat_viewer::handlemsg(msg);
}

bool ht_hex_viewer::pos_to_offset(viewer_pos p, FileOfs *ofs)
{
	*ofs = p.u.line_id.id2 + (uint64(p.u.line_id.id1) << 32) + p.u.tag_idx;
	return true;
}

bool ht_hex_viewer::offset_to_pos(FileOfs ofs, viewer_pos *p)
{
	uint ll = h->get_line_length();
	clear_viewer_pos(p);
	p->u.sub = first_sub;
	p->u.line_id.id1 = (ofs - (ofs % ll)) >> 32;
	p->u.line_id.id2 = ofs - (ofs % ll);
	p->u.tag_idx = ofs % ll;
	return true;
}

bool ht_hex_viewer::qword_to_pos(uint64 q, viewer_pos *p)
{
	int ll = h->get_line_length();
	ht_linear_sub *s = (ht_linear_sub*)cursor.sub;
	FileOfs ofs = q;
	clear_viewer_pos(p);
	p->u.sub = s;
	p->u.tag_idx = ofs % ll;
	return s->convert_ofs_to_id(ofs, &p->u.line_id);
}

bool ht_hex_viewer::symbol_handler(eval_scalar *result, char *name)
{
	if (strcmp(name, "$") == 0) {
		FileOfs ofs;
		if (!pos_to_offset(*(viewer_pos*)&cursor, &ofs)) return 0;
		scalar_create_int_c(result, ofs);
		return true;
	}
	return ht_uformat_viewer::symbol_handler(result, name);
}

/*
 *	CLASS ht_hex_file_sub
 */

void ht_hex_file_sub::handlemsg(htmsg *msg)
{
	if (msg->msg == msg_filesize_changed) {
		fsize = file->getSize();
		return;
	}
	ht_hex_sub::handlemsg(msg);
}

