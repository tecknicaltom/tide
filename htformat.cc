/*
 *	HT Editor
 *	htformat.cc
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

#include "htformat.h"
#include "htsearch.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "blockop.h"
#include "cmds.h"
#include "htapp.h"
#include "htatom.h"
#include "htclipboard.h"
#include "hthist.h"
#include "htiobox.h"
#include "htkeyb.h"
#include "htpal.h"
#include "httag.h"
#include "process.h"
#include "tools.h"

extern "C" {
#include "evalx.h"
#include "regex.h"
}

cmd_rec ht_format_viewer_cmds[] = {
	{cmd_file_truncate, true, true, NULL}
};

/*
 *	CLASS ht_search_request
 */
 

ht_search_request::ht_search_request(UINT _search_class, UINT _type, UINT _flags)
{
	search_class=_search_class;
	type=_type;
	flags=_flags;
}

/*
 *	CLASS ht_search_result
 */
 
ht_search_result::ht_search_result(UINT _search_class)
{
	search_class=_search_class;
}

/*
 *	CLASS ht_physical_search_result
 */
 

ht_physical_search_result::ht_physical_search_result() :
	ht_search_result(SC_PHYSICAL)
{
}

/*
 *	CLASS ht_visual_search_result
 */
 

ht_visual_search_result::ht_visual_search_result() :
	ht_search_result(SC_VISUAL)
{
}

/*
 *	CLASS ht_format_group
 */

void ht_format_group::init(bounds *b, int options, char *desc, ht_streamfile *f, bool own_f, bool editable_f, format_viewer_if **i, ht_format_group *format_group)
{
	ht_format_viewer::init(b, desc, 0, f, format_group);
	VIEW_DEBUG_NAME("ht_format_group");

	xgroup=new ht_xgroup();
	xgroup->init(b, options, desc);
	xgroup->group=group;

	format_views=new ht_clist();	// a list of ht_format_viewer_entrys
	format_views->init();

	own_file=own_f;
	editable_file=editable_f;
	if (i) init_ifs(i);
}

void ht_format_group::done()
{
	done_ifs();

	format_views->destroy();
	delete format_views;

	xgroup->done();
	delete xgroup;

	ht_format_viewer::done();

	if (own_file && file) {
		file->done();
		delete file;
	}
}

int ht_format_group::childcount()
{
	return xgroup->childcount();
}

bool ht_format_group::done_if(format_viewer_if *i, ht_view *v)
{
	remove(v);
	if (i->done) i->done(v); else {
		v->done();
		delete v;
	}
	return 1;
}

void ht_format_group::done_ifs()
{
	int j=0;
	while (1) {
		ht_format_viewer_entry *e=(ht_format_viewer_entry*)format_views->get(j);
		if (!(e && e->instance)) break;
		done_if(e->interface, e->instance);
		j++;
	}
}

bool ht_format_group::edit()
{
	return (file->get_access_mode() & FAM_WRITE);
}

int ht_format_group::focus(ht_view *view)
{
	int r=ht_format_viewer::focus(view);
	if (!r) r=xgroup->focus(view);
	return r;
}

char *ht_format_group::func(UINT i, bool execute)
{
	return ht_format_viewer::func(i, execute);
}

void ht_format_group::getbounds(bounds *b)
{
	xgroup->getbounds(b);
}

void *ht_format_group::get_shared_data()
{
	return shared_data;
}

ht_view *ht_format_group::getfirstchild()
{
	return xgroup->getfirstchild();
}

ht_view *ht_format_group::getselected()
{
	return xgroup->getselected();
}

void ht_format_group::get_pindicator_str(char *buf)
{
	ht_view *c=xgroup->current;
	if (c && (c->options & VO_FORMAT_VIEW)) {
		((ht_format_viewer*)c)->get_pindicator_str(buf);
	} else {
		*buf=0;
	}
}

bool ht_format_group::get_hscrollbar_pos(int *pstart, int *psize)
{
	ht_view *c=xgroup->current;
	if (c && (c->options & VO_FORMAT_VIEW)) {
		return ((ht_format_viewer*)c)->get_hscrollbar_pos(pstart, psize);
	}
	return false;
}

bool ht_format_group::get_vscrollbar_pos(int *pstart, int *psize)
{
	ht_view *c=xgroup->current;
	if (c && (c->options & VO_FORMAT_VIEW)) {
		return ((ht_format_viewer*)c)->get_vscrollbar_pos(pstart, psize);
	}
	return false;
}

void ht_format_group::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
		case msg_keypressed: {
			int i=0;
			switch (msg->data1.integer) {
				case K_F12: i++;
				case K_F11: i++;
				case K_F10: i++;
				case K_F9: i++;
				case K_F8: i++;
				case K_F7: i++;
				case K_F6: i++;
				case K_F5: i++;
				case K_F4: i++;
				case K_F3: i++;
				case K_F2: i++;
				case K_F1: {
					i++;
					htmsg m;
					m.msg=msg_funcquery;
					m.type=mt_empty;
					m.data1.integer=i;
					sendmsg(&m);
					if (m.msg==msg_retval) {
						sendmsg(msg_funcexec, i);
						clearmsg(msg);
						return;
					}
					break;
				}
			}
			break;
		}
	}
	ht_format_viewer::handlemsg(msg);
	xgroup->handlemsg(msg);
	switch (msg->msg) {
		case msg_funcexec:
			if (func(msg->data1.integer, 1)) {
				clearmsg(msg);
				return;
			}
			break;
		case msg_funcquery: {
			char *s=func(msg->data1.integer, 0);
			if (s) {
				msg->msg=msg_retval;
				msg->data1.str=s;
			}
			break;
		}
	}
}

bool ht_format_group::init_if(format_viewer_if *i)
{
	bounds b;
	getbounds(&b);
	b.x=0;
	b.y=0;
	bool r=0;
	ht_view *v=0;
	
/*     bounds c=*b;

	c.x=c.w-1;
	c.y=0;
	c.w=1;*/
	if (i->init) {
		v=i->init(&b, file, this);
		if (v) {
			v->sendmsg(msg_complete_init, 0);
			insert(v);
			r=1;
		}
	}
	ht_format_viewer_entry *e=new ht_format_viewer_entry();
	e->interface=i;
	e->instance=v;
	format_views->insert(e);
	return r;
}

void ht_format_group::init_ifs(format_viewer_if **ifs)
{
	format_viewer_if **i=ifs;
	while (*i) {
		init_if(*i);
		i++;
	}
	ifs=i;
}

void ht_format_group::insert(ht_view *view)
{
	xgroup->insert(view);
}

void ht_format_group::move(int rx, int ry)
{
	ht_format_viewer::move(rx, ry);
	xgroup->move(rx, ry);
}

void ht_format_group::receivefocus()
{
	xgroup->receivefocus();
}

void ht_format_group::redraw()
{
	xgroup->redraw();
}

void ht_format_group::releasefocus()
{
	xgroup->releasefocus();
}

void ht_format_group::remove(ht_view *view)
{
	xgroup->remove(view);
}

void ht_format_group::resize(int rw, int rh)
{
	ht_format_viewer::resize(rw, rh);
	xgroup->resize(rw, rh);
}

void ht_format_group::setgroup(ht_group *_group)
{
	xgroup->setgroup(_group);
}

/*
 *	CLASS ht_viewer
 */

void ht_viewer::init(bounds *b, char *desc, UINT _caps)
{
	ht_view::init(b, VO_OWNBUFFER | VO_BROWSABLE | VO_SELECTABLE | VO_MOVE | VO_RESIZE, desc);
	caps=_caps;
	
	growmode=GM_VDEFORM | GM_HDEFORM;
}

void ht_viewer::done()
{
	ht_view::done();
}

char *ht_viewer::func(UINT i, bool execute)
{
	return 0;
}

void ht_viewer::handlemsg(htmsg *msg)
{
	int i=0;
	switch (msg->msg) {
		case msg_keypressed: {
			switch (msg->data1.integer) {
				case K_F12: i++;
				case K_F11: i++;
				case K_F10: i++;
				case K_F9: i++;
				case K_F8: i++;
				case K_F7: i++;
				case K_F6: i++;
				case K_F5: i++;
				case K_F4: i++;
				case K_F3: i++;
				case K_F2: i++;
				case K_F1: {
					i++;
					htmsg m;
					m.msg=msg_funcquery;
					m.type=mt_empty;
					m.data1.integer=i;
					sendmsg(&m);
					if (m.msg==msg_retval) {
						sendmsg(msg_funcexec, i);
						clearmsg(msg);
						return;
					}
					break;
				}
			}
			break;
		}
		case msg_funcexec:
			if (func(msg->data1.integer, 1)) {
				clearmsg(msg);
				return;
			}
			break;
		case msg_funcquery: {
			char *s=func(msg->data1.integer, 0);
			if (s) {
				msg->msg=msg_retval;
				msg->data1.str=s;
			}
			break;
		}
	}
	ht_view::handlemsg(msg);
}

/*
 *	CLASS ht_format_viewer
 */

void ht_format_viewer::init(bounds *b, char *desc, UINT caps, ht_streamfile *f, ht_format_group *fg)
{
	ht_viewer::init(b, desc, caps);
	options |= VO_FORMAT_VIEW;
	VIEW_DEBUG_NAME("ht_format_viewer");
	file = f;
	format_group = fg;

	last_search_request = 0;

	vs_history = new ht_stack();
	vs_history->init();
}

void ht_format_viewer::done()
{
	UINT i = 0;
	vstate *v;
	while ((v = (vstate*)vs_history->get(i))) {
		view_state_destroy(v->data);
		i++;
	}
	vs_history->destroy();
	delete vs_history;

	ht_view::done();
}

bool ht_format_viewer::address_to_offset(fmt_vaddress addr, FILEOFS *ofs)
{
	return 0;
}

bool ht_format_viewer::continue_search()
{
	if (last_search_request) {
		ht_search_result *r=0;
		if (last_search_physical) {
			FILEOFS o, no;
			if (get_current_offset(&o)) {
				try {
					if (last_search_request->search_class==SC_VISUAL) {
						if (next_logical_offset(o, &no)) {
							r=psearch(last_search_request, no, last_search_end_ofs);
						}
					} else {
						r=psearch(last_search_request, o+1, last_search_end_ofs);
					}
				} catch (ht_exception *e) {
					errorbox("error: %s", e->what());
				}
			}
		} else {
			fmt_vaddress a, na;
			if (get_current_address(&a)) {
				try {
					if (last_search_request->search_class==SC_VISUAL) {
						if (next_logical_address(a, &na)) {
							r=vsearch(last_search_request, na, last_search_end_addr);
						}
					} else {
						r=vsearch(last_search_request, a+1, last_search_end_addr);
					}
				} catch (ht_exception *e) {
					errorbox("error: %s", e->what());
				}
			}
		}
		
		if (r) return show_search_result(r);
	}
	return false;
}

bool ht_format_viewer::get_current_address(fmt_vaddress *addr)
{
	return false;
}

bool ht_format_viewer::get_current_offset(FILEOFS *ofs)
{
	return 0;
}

ht_streamfile *ht_format_viewer::get_file()
{
	return file;
}

void ht_format_viewer::get_pindicator_str(char *buf)
{
	*buf=0;
}

bool ht_format_viewer::get_hscrollbar_pos(int *pstart, int *psize)
{
	return false;
}

bool ht_format_viewer::get_vscrollbar_pos(int *pstart, int *psize)
{
	return false;
}

bool ht_format_viewer::goto_address(fmt_vaddress addr, ht_view *source_object)
{
	return 0;
}

bool ht_format_viewer::goto_offset(FILEOFS ofs, ht_view *source_object)
{
	return 0;
}

void ht_format_viewer::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
		case msg_vs_restore:
			view_state_load(msg->data1.ptr);
			clearmsg(msg);
			return;
		case cmd_file_truncate: {
			ht_streamfile *f=(ht_streamfile *)msg->data1.ptr;
			FILEOFS o=(FILEOFS)msg->data2.integer;
			if (file==f) {
				ht_format_loc loc;
				loc_enum_start();
				while (loc_enum_next(&loc)) {
					if (o < loc.start+loc.length) {
						if (confirmbox("truncating at %08x will destroy format '%s', continue ? \n(format ranges from %08x to %08x)", o, loc.name, loc.start, loc.start+loc.length) != button_yes) {
							clearmsg(msg);
							return;
						}
						break;
					}
				}
			}
			break;
		}
		case cmd_edit_mode_i:
			if (file/* && (file==msg->data1.ptr)*/) {
				if (file->set_access_mode(FAM_READ | FAM_WRITE)) {
					htmsg m;
					m.msg=cmd_edit_mode;
					m.type=mt_broadcast;
					sendmsg(&m);
				} else errorbox("can't open file %s in write mode ! (error %08x)", file->get_filename(), file->get_error());
			}
			clearmsg(msg);
			return;
		case cmd_view_mode_i:
			if (file /*&& (file==msg->data1.ptr)*/) {
				UINT size = file->get_size();
				file->cntl(FCNTL_MODS_INVD);
				if (file->set_access_mode(FAM_READ)) {
					htmsg m;
					m.msg=cmd_view_mode;
					m.type=mt_broadcast;
					sendmsg(&m);
				} else errorbox("can't (re)open file %s in read mode ! (error %08x)", file->get_filename(), file->get_error());
				if (size != file->get_size()) {
					htmsg m;
					m.msg=msg_filesize_changed;
					m.type=mt_broadcast;
					sendmsg(&m);
				}
			}
			clearmsg(msg);
			return;
	}
	ht_viewer::handlemsg(msg);
}

void ht_format_viewer::loc_enum_start()
{
}

bool ht_format_viewer::loc_enum_next(ht_format_loc *loc)
{
	return 0;
}

bool ht_format_viewer::next_logical_address(fmt_vaddress addr, fmt_vaddress *naddr)
{
	return 0;
}

bool ht_format_viewer::next_logical_offset(FILEOFS ofs, FILEOFS *nofs)
{
	return 0;
}

bool ht_format_viewer::offset_to_address(FILEOFS ofs, fmt_vaddress *addr)
{
	return 0;
}

bool ht_format_viewer::push_vs_history(ht_view *focused)
{
	void *vs=view_state_create();
	if (vs) {
		vstate *v = new vstate();
		v->focused = focused;
		v->view = this;
		v->data = vs;
		vs_history->push(v);
		return 1;
	}
	return 0;
}

bool ht_format_viewer::pop_vs_history()
{
	vstate *vs=(vstate*)vs_history->pop();
	if (vs) {
		baseview->focus(vs->focused);
		htmsg m;
		m.msg = msg_vs_restore;
		m.type = mt_empty;
		m.data1.ptr = vs->data;
		vs->view->sendmsg(&m);
		delete vs;
		return true;
	}
	return false;
}

UINT ht_format_viewer::pread(FILEOFS ofs, void *buf, UINT size)
{
	if (file->seek(ofs)==0) {
		return file->read(buf, size);
	}
	return 0;
}

ht_search_result *ht_format_viewer::psearch(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	return 0;
}

void ht_format_viewer::pselect_add(FILEOFS start, FILEOFS end)
{
}

void ht_format_viewer::pselect_get(FILEOFS *start, FILEOFS *end)
{
}

void ht_format_viewer::pselect_set(FILEOFS start, FILEOFS end)
{
}

UINT ht_format_viewer::pwrite(FILEOFS ofs, void *buf, UINT size)
{
	if (file->seek(ofs)==0) {
		sendmsg(msg_file_changed);
		return file->write(buf, size);
	}
	return 0;
}

bool ht_format_viewer::show_search_result(ht_search_result *r)
{
	switch (r->search_class) {
		case SC_PHYSICAL: {
			ht_physical_search_result *s=(ht_physical_search_result*)r;
			if (!goto_offset(s->offset, this)) return false;
			pselect_set(s->offset, s->offset+s->size);
			return true;
		}
		case SC_VISUAL: {
			ht_visual_search_result *s=(ht_visual_search_result*)r;
			return goto_address(s->address, this);
		}
	}
	return false;
}

bool ht_format_viewer::string_to_address(char *string, fmt_vaddress *addr)
{
	return false;
}

bool ht_format_viewer::string_to_offset(char *string, FILEOFS *ofs)
{
	return false;
}

void *ht_format_viewer::view_state_create()
{
	return NULL;
}

void ht_format_viewer::view_state_destroy(void *view_state)
{
	free(view_state);
}

void ht_format_viewer::view_state_load(void *view_state)
{
}

UINT ht_format_viewer::vread(fmt_vaddress addr, void *buf, UINT size)
{
	FILEOFS o;
	if (address_to_offset(addr, &o)) {
		return pread(o, buf, size);
	}
	return 0;
}

ht_search_result *ht_format_viewer::vsearch(ht_search_request *search, fmt_vaddress start, fmt_vaddress end)
{
	FILEOFS so, eo;
	if (address_to_offset(start, &so) && address_to_offset(end, &eo)) {
		ht_search_result *r=psearch(search, so, eo);
		last_search_physical=0;
		last_search_end_addr=end;
		return r;
	}
	return 0;
}

void ht_format_viewer::vselect_add(fmt_vaddress start, fmt_vaddress end)
{
	FILEOFS so, eo;
	if (address_to_offset(start, &so) && address_to_offset(end, &eo)) {
		return pselect_add(so, eo);
	}
}

void ht_format_viewer::vselect_get(fmt_vaddress *start, fmt_vaddress *end)
{
	HT_ERROR("nyi!");
}

void ht_format_viewer::vselect_set(fmt_vaddress start, fmt_vaddress end)
{
	FILEOFS so, eo;
	if (address_to_offset(start, &so) && address_to_offset(end, &eo)) {
		return pselect_set(so, eo);
	}
}

UINT ht_format_viewer::vwrite(fmt_vaddress vaddr, void *buf, UINT size)
{
	FILEOFS o;
	if (address_to_offset(vaddr, &o)) {
		return pwrite(o, buf, size);
	}
	return 0;
}

/*
 *	CLASS ht_uformat_view
 */

void ht_uformat_viewer::init(bounds *b, char *desc, int caps, ht_streamfile *file, ht_format_group *format_group)
{
	tagpal.data=NULL;
	tagpal.size=0;
	ht_format_viewer::init(b, desc, caps, file, format_group);
	VIEW_DEBUG_NAME("ht_uformat_view");
	first_sub=0;
	last_sub=0;
	cursor_sub=0;
	top_sub=0;
	xscroll=0;
	cursor_ypos=0;
	cursor_visual_length=0;
	cursor_visual_xpos=0;
	cursor_select=0;
	cursor_select_start=0xffffffff;
	sel_start=0;
	sel_end=0;
	isdirty_cursor_line=0;

	search_caps=SEARCHMODE_VREGEX;

	uf_initialized=0;
}

void ht_uformat_viewer::done()
{
	edit_end();
	clear_subs();
	free(tagpal.data);
	ht_format_viewer::done();
}

void ht_uformat_viewer::adjust_cursor_group()
{
	cursorline_get();
	int g=tag_count_groups(cursor_line);
	if (cursor_tag_group>=g) cursor_tag_group=0;
}

void ht_uformat_viewer::adjust_cursor_idx()
{
	cursorline_get();
	int c=tag_count_selectable_tags_in_group(cursor_line, cursor_tag_group);
	if (cursor_tag_idx>c-1) cursor_tag_idx=c-1;
}

int ht_uformat_viewer::center_view(ht_sub *sub, ID id1, ID id2)
{
	int r=prev_line(&sub, &id1, &id2, size.h/2);
	top_sub=sub;
	top_id1=id1;
	top_id2=id2;
	cursorline_dirty();
	return r;
}

void ht_uformat_viewer::check_cursor_visibility()
{
	if (cursor_state!=cursor_state_disabled) {
		if ((cursor_ypos<0) || (cursor_ypos>=size.h)) {
			cursor_state=cursor_state_invisible;
		} else {
			cursor_state=cursor_state_visible;
		}
	}
}

void ht_uformat_viewer::complete_init()
{
	if (uf_initialized) return;
	cursor_state=cursor_state_disabled;
	if (!first_sub) {
		uf_initialized=1;
		return;
	}
/*  initialize top_*  */
	top_sub=first_sub;
	top_sub->first_line_id(&top_id1, &top_id2);
/*  initialize cursor_*  */
	cursor_tag_idx=0;
	cursor_tag_group=0;
	cursor_tag_micropos=0;

	cursor_sub=first_sub;
	cursor_sub->first_line_id(&cursor_id1, &cursor_id2);
	char line[1024];	/* FIXME: possible buffer overflow ! */
	cursor_ypos--;
	do {
		cursor_ypos++;
		if (!cursor_sub->getline(line, cursor_id1, cursor_id2)) break;
		if (tag_count_selectable_tags(line)) {
			if (cursor_ypos<size.h) {
				cursor_state=cursor_state_visible;
			} else {
				cursor_state=cursor_state_invisible;
				cursor_ypos=-1;
			}
			break;
		}
	} while ((next_line(&cursor_sub, &cursor_id1, &cursor_id2, 1)) && (cursor_ypos<size.h));
	if (cursor_state==cursor_state_disabled) {
		cursor_sub=first_sub;
		cursor_sub->first_line_id(&cursor_id1, &cursor_id2);
		if (cursor_sub->getline(line, cursor_id1, cursor_id2)) {
			cursor_ypos=-1;
			cursor_state=cursor_state_invisible;
		}
	}
/* get cursorline */
	cursorline_dirty();
	cursorline_get();
/* initialize visual */
	update_visual_info();
/* initialize misc */
	update_misc_info();

	uf_initialized=1;
}

int ht_uformat_viewer::cursor_left()
{
	if (cursor_tag_idx) {
		cursor_tag_idx--;
		update_visual_info();
		update_misc_info();
		return 1;
	} else {
		if (cursor_up(1)) {
			cursor_end();
			return 1;
		}
	}
	return 0;
}

int ht_uformat_viewer::cursor_right()
{
	cursorline_get();
	if (cursor_tag_idx<tag_count_selectable_tags_in_group(cursor_line, cursor_tag_group)-1) {
		cursor_tag_idx++;
		update_visual_info();
		update_misc_info();
		return 1;
	} else {
		if (cursor_down(1)) {
			cursor_home();
			return 1;
		}
	}
	return 0;
}

int ht_uformat_viewer::cursor_up(int n)
{
	switch (cursor_state) {
		case cursor_state_invisible:
		case cursor_state_visible: {
			if ((n==1) && (cursor_state==cursor_state_visible)) {
				int r=0;
				ht_sub *c_sub=cursor_sub;
				ID c_id1=cursor_id1, c_id2=cursor_id2;
				char c_line[1024];
				int c_ypos=cursor_ypos;
				int c_tag_idx=cursor_tag_idx;
				int c_tag_group=cursor_tag_group;
				int d_tag_group=cursor_tag_group;

				while (prev_line(&c_sub, &c_id1, &c_id2, 1) && (c_ypos>=0)) {
					c_ypos--;
					c_sub->getline(c_line, c_id1, c_id2);
					int g=tag_count_groups(c_line);
					if (d_tag_group<g) c_tag_group=d_tag_group;
					int s;
					if (c_tag_group>=g) {
						c_tag_group=g-1;
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							c_tag_idx=s-1;
							r=1;
							break;
						}
					} else {
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							if (c_tag_idx>=s) c_tag_idx=s-1;
							r=1;
							break;
						}
					}
				}
				if (r) {
					cursor_sub=c_sub;
					cursor_id1=c_id1;
					cursor_id2=c_id2;
					memmove(cursor_line, c_line, sizeof cursor_line);
					cursorline_dirty();
					cursor_ypos=c_ypos;
					cursor_tag_idx=c_tag_idx;
					cursor_tag_group=c_tag_group;
					if (cursor_ypos<=-1) scroll_up(-cursor_ypos);
					update_misc_info();
					update_visual_info();
					if (edit()) update_micropos();
				} else scroll_up(n);
				return r;
			} else {
				int r=0;
				char c_line[1024];
				int c_tag_idx=cursor_tag_idx;
				int c_tag_group=cursor_tag_group;
				int d_tag_group=cursor_tag_group;
				ht_sub *c_sub;
				ID c_id1;
				ID c_id2;
				int c_ypos;
				if (cursor_state==cursor_state_invisible) {
					c_sub=top_sub;
					c_id1=top_id1;
					c_id2=top_id2;
					c_ypos=0;
				} else {
					c_sub=cursor_sub;
					c_id1=cursor_id1;
					c_id2=cursor_id2;
					c_ypos=cursor_ypos;
				}
				int nc=prev_line(&c_sub, &c_id1, &c_id2, n);
				c_ypos-=nc;

				while (nc--) {
					c_sub->getline(c_line, c_id1, c_id2);
					int g=tag_count_groups(c_line);
					if (d_tag_group<g) c_tag_group=d_tag_group;
					int s;
					if (c_tag_group>=g) {
						c_tag_group=g-1;
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							c_tag_idx=s-1;
							r=1;
							break;
						}
					}
					s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
					if (s) {
						if (c_tag_idx>=s) c_tag_idx=s-1;
						r=1;
						break;
					}
					if (!next_line(&c_sub, &c_id1, &c_id2, 1)) break;
					c_ypos++;
				}
				if (r) {
					cursor_sub=c_sub;
					cursor_id1=c_id1;
					cursor_id2=c_id2;
					memmove(cursor_line, c_line, sizeof cursor_line);
					cursorline_dirty();
					cursor_ypos=c_ypos;
					cursor_tag_idx=c_tag_idx;
					cursor_tag_group=c_tag_group;
					if (-cursor_ypos+n-nc-1>0) scroll_up(-cursor_ypos+n-nc-1);
					update_misc_info();
					update_visual_info();
					if (edit()) update_micropos();
				} else {
					if (cursor_state==cursor_state_invisible) cursor_ypos=-0x80000000;
					scroll_up(n);
				}
				break;
			}
		}
		case cursor_state_disabled:
//			scroll_up(n);
//			return n;
			break;
	}
	return 0;
}

int ht_uformat_viewer::cursor_down(int n)
{
	switch (cursor_state) {
		case cursor_state_invisible:
		case cursor_state_visible: {
			if ((n==1) && (cursor_state==cursor_state_visible)) {
				int r=0;
				ht_sub *c_sub=cursor_sub;
				ID c_id1=cursor_id1, c_id2=cursor_id2;
				char c_line[1024];
				int c_ypos=cursor_ypos;
				int c_tag_idx=cursor_tag_idx;
				int c_tag_group=cursor_tag_group;
				int d_tag_group=cursor_tag_group;
				int nls;	/* controls scrolling beyond end */

				while ((nls=next_line(&c_sub, &c_id1, &c_id2, 1)) && (c_ypos<=size.h-1)) {
					c_ypos++;
					c_sub->getline(c_line, c_id1, c_id2);
					int g=tag_count_groups(c_line);
					if (d_tag_group<g) c_tag_group=d_tag_group;
					int s;
					if (c_tag_group>=g) {
						c_tag_group=g-1;
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							c_tag_idx=s-1;
							r=1;
							break;
						}
					}
					s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
					if (s) {
						if (c_tag_idx>=s) c_tag_idx=s-1;
						r=1;
						break;
					}
				}
				if (r) {
					cursor_sub=c_sub;
					cursor_id1=c_id1;
					cursor_id2=c_id2;
					memmove(cursor_line, c_line, sizeof cursor_line);
					cursorline_dirty();
					cursor_ypos=c_ypos;
					cursor_tag_idx=c_tag_idx;
					cursor_tag_group=c_tag_group;
					if (cursor_ypos>=size.h) scroll_down(cursor_ypos-size.h+1);
					update_misc_info();
					update_visual_info();
					if (edit()) update_micropos();
				} else if (nls) scroll_down(1);
				return r;
			} else {
				int r=0;
				char c_line[1024];
				int c_tag_idx=cursor_tag_idx;
				int c_tag_group=cursor_tag_group;
				int d_tag_group=cursor_tag_group;
				ht_sub *c_sub;
				ID c_id1;
				ID c_id2;
				int c_ypos;
				if (cursor_state==cursor_state_invisible) {
					c_sub=top_sub;
					c_id1=top_id1;
					c_id2=top_id2;
					c_ypos=next_line(&c_sub, &c_id1, &c_id2, size.h-1);
				} else {
					c_sub=cursor_sub;
					c_id1=cursor_id1;
					c_id2=cursor_id2;
					c_ypos=cursor_ypos;
				}

				int nc=next_line(&c_sub, &c_id1, &c_id2, n);
				int onc=c_ypos+nc-size.h+1;
				c_ypos+=nc;

				while (nc--) {
					c_sub->getline(c_line, c_id1, c_id2);
					int g=tag_count_groups(c_line);
					if (d_tag_group<g) c_tag_group=d_tag_group;
					int s;
					if (c_tag_group>=g) {
						c_tag_group=g-1;
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							c_tag_idx=s-1;
							r=1;
							break;
						}
					} else {
						s=tag_count_selectable_tags_in_group(c_line, c_tag_group);
						if (s) {
							if (c_tag_idx>=s) c_tag_idx=s-1;
							r=1;
							break;
						}
					}
					if (nc) if (!prev_line(&c_sub, &c_id1, &c_id2, 1)) break;
					c_ypos--;
				}
				if (r) {
					cursor_sub=c_sub;
					cursor_id1=c_id1;
					cursor_id2=c_id2;
					memmove(cursor_line, c_line, sizeof cursor_line);
					cursorline_dirty();
					cursor_ypos=c_ypos;
					cursor_tag_idx=c_tag_idx;
					cursor_tag_group=c_tag_group;
					if (cursor_ypos-size.h+1>0) scroll_down(cursor_ypos-size.h+1);
					update_misc_info();
					update_visual_info();
					if (edit()) update_micropos();
				} else if (onc>0) {
					if (cursor_state==cursor_state_invisible) cursor_ypos=-0x80000000;
					scroll_down(onc);
				}
				break;
			}
		}
		case cursor_state_disabled:
//			scroll_down(n);
//			return n;
			break;
	}
	return 0;
}

int ht_uformat_viewer::cursor_home()
{
	if (cursor_tag_idx) {
		cursor_tag_idx=0;
	}
	if (edit()) cursor_tag_micropos=0;
	update_visual_info();
	update_misc_info();
	return 1;
}

int ht_uformat_viewer::cursor_end()
{
	cursorline_get();
	int c=tag_count_selectable_tags_in_group(cursor_line, cursor_tag_group);
	if (cursor_tag_idx!=c-1) {
		cursor_tag_idx=c-1;
	}
	if (edit()) {
		char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
		if (e) cursor_tag_micropos=tag_get_microsize(e)-1;
	}
	update_visual_info();
	update_misc_info();
	return 1;
}

void ht_uformat_viewer::cursor_tab()
{
	cursor_tag_group++;
	adjust_cursor_group();
}

void ht_uformat_viewer::cursorline_dirty()
{
	isdirty_cursor_line=1;
}

void ht_uformat_viewer::cursorline_get()
{
	if (isdirty_cursor_line) {
		if (cursor_sub) cursor_sub->getline(cursor_line, cursor_id1, cursor_id2);
		isdirty_cursor_line=0;
	}
}

int ht_uformat_viewer::cursormicroedit_forward()
{
	cursorline_get();
	ht_sub *sub=top_sub;
	ID id1=top_id1;
	ID id2=top_id2;
	UINT cursor_tag_bitidx = 0;

	cursorline_get();
	char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	if (e) {
		if (((byte*)e)[1] == HT_TAG_EDIT_BIT) {
			cursor_tag_bitidx = ((ht_tag_edit_bit*)e)->bitidx;
		}
	}

	bool cursor_found = false;
	int tag_idx=0;
	char c_line[1024];  /* FIXME: possible buffer overflow ! */
	int g=cursor_tag_group;
	for (int y=0; y<size.h+16; y++) {
		if (!sub->getline(c_line, id1, id2)) break;
		int c=tag_count_selectable_tags_in_group(c_line, cursor_tag_group);
		while (tag_idx<c) {
			char *t=tag_get_selectable_tag(c_line, tag_idx, cursor_tag_group);
			if (tag_get_class(t)==tag_class_edit) {
				if (cursor_found) {
					cursor_tag_micropos=0;
					set_cursor(sub, id1, id2);
					cursorline_dirty();
					cursor_tag_group=g;
					cursor_tag_idx=tag_idx;
					update_misc_info();
					update_visual_info();
					return 1;
				} else if (tag_get_offset(t) == cursor_tag_offset) {
					if ( ( (((byte*)t)[1] == HT_TAG_EDIT_BIT) &&
					( ((ht_tag_edit_bit*)t)->bitidx == cursor_tag_bitidx) ) ||
					(((byte*)t)[1] != HT_TAG_EDIT_BIT)) {
						cursor_found = true;
						char *t=tag_get_selectable_tag(c_line, tag_idx, cursor_tag_group);
						int s=tag_get_microsize(t);
						if (cursor_tag_micropos+1 < s) {
							cursor_tag_micropos++;
							set_cursor(sub, id1, id2);
							cursorline_dirty();
							cursor_tag_group=g;
							cursor_tag_idx=tag_idx;
							update_misc_info();
							update_visual_info();
							return 1;
						}
					}
				}
			}
			tag_idx++;
		}
		tag_idx=0;
		if (!next_line(&sub, &id1, &id2, 1)) break;
	}
	if (cursor_right()) cursor_tag_micropos=0;
	return 0;
}

int ht_uformat_viewer::cursormicro_forward()
{
	cursorline_get();
	char *t=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	int s=tag_get_microsize(t);
	if (cursor_tag_micropos+1>=s) {
		if (cursor_right()) cursor_tag_micropos=0; else return 0;
	} else cursor_tag_micropos++;
	return 1;
}

int ht_uformat_viewer::cursormicro_backward()
{
	if (cursor_tag_micropos>0) {
		cursor_tag_micropos--;
	} else {
		if (cursor_left()) {
			cursorline_get();
			char *t=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
			cursor_tag_micropos=tag_get_microsize(t)-1;
		} else return 0;
	}
	return 1;
}

void ht_uformat_viewer::draw()
{
	int max_sdown_count=0;
	if (!uf_initialized) HT_ERROR("complete_init() not called !");
restart:
	clear(getcolor(palidx_generic_body));
	if (!first_sub) return;
	ID id1=top_id1, id2=top_id2;
	ht_sub *sub=top_sub;
	ID pid1=0, pid2=0;
	ht_sub *psub=0;
	char line[1024];	/* FIXME: possible buffer overflow ! */
	int cursor_in_line, cursor_found=-1;
	if (focused) hidecursor();
	if (!sub->getline(line, id1, id2)) {
		if (sub->closest_line_id(&id1, &id2)) {
			top_id1 = id1;
			top_id2 = id2;
		} else return;
		if (!sub->getline(line, id1, id2)) return;
	}
	for (int y=0; y<size.h; y++) {
		if (y==cursor_ypos) {
			pid1=id1;
			pid2=id2;
			psub=sub;
		}
		if (!sub->getline(line, id1, id2)) break;
/**/
/*		char test[128];
		tag_striptags(test+sprintf(test, "%08x,%08x: ", id1, id2), line);
		fprintf(stdout, "%s\n", test);
		fflush(stdout);*/
/**/
		if ((cursor_state==cursor_state_visible) && (sub==cursor_sub) &&
		(id1==cursor_id1) && (id2==cursor_id2)) {
			cursor_found=y;
			cursorline_dirty();
			update_misc_info();
			int c=tag_count_selectable_tags_in_group(line, cursor_tag_group);
			if (cursor_tag_idx>=c) cursor_tag_idx=c-1;
			if ((cursor_tag_class==tag_class_edit) && (edit())) {
				char *t=tag_get_selectable_tag(line, cursor_tag_idx, cursor_tag_group);
				int x=cursor_visual_xpos+tag_get_micropos(t, cursor_tag_micropos)-xscroll;
				if (focused) if (x>=0) setcursor(x, y);
			}
			cursor_in_line=1;
		} else cursor_in_line=0;
		print_tagstring(0, y, size.w, xscroll, line, cursor_in_line);
		if (xscroll>0)	buf_printchar(0, y, VCP(VC_GREEN, VC_TRANSPARENT), '<');
		if (!next_line(&sub, &id1, &id2, 1)) break;
	}
	if ((cursor_state==cursor_state_visible) && (cursor_found==-1)) {
/*		scroll_down(1);
		max_sdown_count++;
		if (max_sdown_count>=size.h) assert(0);
		goto restart;*/
		if (!psub) {
			pid1=top_id1;
			pid2=top_id2;
			psub=top_sub;
		}
		cursor_id1=pid1;
		cursor_id2=pid2;
		cursor_sub=psub;
		cursorline_dirty();
		update_misc_info();
		int c=tag_count_selectable_tags_in_group(line, cursor_tag_group);
		if (cursor_tag_idx>c-1) cursor_tag_idx=c-1;

		if (max_sdown_count++) {

		} else goto restart;
	}
	if (cursor_found!=-1) cursor_ypos=cursor_found;
//	buf_printf(1, 1, VCP(VC_GREEN, VC_BLACK), "%s", focused ? "focused" : "unfocused");
//	if (cursor_select) printf(20, 1, VCP(VC_GREEN, VC_BLACK), "start=%08x, l=%08x", cursor_select_start, cursor_select_cursor_length);
//	buf_printf(20, 6, 7, "csize.x=%2d, csize.y=%2d, csize.w=%2d, csize.h=%2d", csize.x, csize.y, csize.w, csize.h);
//	buf_printf(2, 3, 7, "size.x=%2d, size.y=%2d, size.w=%2d, size.h=%2d", size.x, size.y, size.w, size.h);
//	buf_printf(2, 4, 7, "vsize.x=%2d, vsize.y=%2d, vsize.w=%2d, vsize.h=%2d", vsize.x, vsize.y, vsize.w, vsize.h);
//	buf_printf(2, 5, 7, "bsize.x=%2d, bsize.y=%2d, bsize.w=%2d, bsize.h=%2d", buf->size.x, buf->size.y, buf->size.w, buf->size.h);
//	buf_printf(20, 7, 7, "size.x=%2d, size.y=%2d, size.w=%2d, size.h=%2d", group->group->size.x, group->group->size.y, group->group->size.w, group->group->size.h);
//	buf_printf(20, 8, 7, "vsize.x=%2d, vsize.y=%2d, vsize.w=%2d, vsize.h=%2d", group->group->vsize.x, group->group->vsize.y, group->group->vsize.w, group->group->vsize.h);
//	buf_printf(0, 7, 7, "vx=%2d, vl=%2d, ypos=%2d", cursor_visual_xpos, cursor_visual_length, cursor_ypos);
//	buf_printf(0, 2, 7, "cursor_micropos=%d", cursor_tag_micropos);
//	buf_printf(0, 2, 7, "cursor_id1=%x, cursor_id2=%x", cursor_id1, cursor_id2);
//	buf_printf(0, 3, 7, "(%c)", "vid"[cursor_state]);
//	buf_printf(0, 6, 7, "cursor_tag_idx=%d", cursor_tag_idx);
/*     if (cursor_tag_class==tag_class_edit) {
		printf(30, 7, 7, "class=edit, addr=%08x", cursor_tag_offset);
	} else {
		printf(30, 7, 7, "class=sel, id_low=%08x, id_high=%08x", cursor_tag_id.low, cursor_tag_id.high);
	}*/
}

bool ht_uformat_viewer::edit()
{
	return (file && (file->get_access_mode() & FAM_WRITE));
}

bool ht_uformat_viewer::edit_end()
{
	if (!edit()) {
		hidecursor();
		if (find_first_tag(&cursor_sub, &cursor_id1, &cursor_id2, size.h)) {
			set_cursor(cursor_sub, cursor_id1, cursor_id2);
		} else {
//			if (cursor_state!=cursor_state_disabled) {
/* is that all we have to do ??? testing needed */
//				cursor_state=cursor_state_invisible;
//				assert(0);	/* FIXME: not yet implemented */
//			}
		}
		cursorline_dirty();
		adjust_cursor_idx();
		update_visual_info();
		update_misc_info();
		dirtyview();
		return true;
	}
	return false;
}

bool ht_uformat_viewer::edit_input(byte b)
{
	cursorline_get();
	char *t=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	switch (t[1]) {
		case HT_TAG_EDIT_BYTE:
		case HT_TAG_EDIT_WORD_LE:
		case HT_TAG_EDIT_DWORD_LE:
		case HT_TAG_EDIT_QWORD_LE:
		case HT_TAG_EDIT_WORD_BE:
		case HT_TAG_EDIT_DWORD_BE:
		case HT_TAG_EDIT_QWORD_BE: {
			int nibval=edit_input_c2h(b);
			if (nibval==-1) break;
			
			int size=0;
			bool bigendian = true;
			switch (t[1]) {
				case HT_TAG_EDIT_BYTE: size=1; break;
				case HT_TAG_EDIT_WORD_LE: size=2; bigendian=false; break;
				case HT_TAG_EDIT_DWORD_LE: size=4; bigendian=false; break;
				case HT_TAG_EDIT_QWORD_LE: size=8; bigendian=false; break;
				case HT_TAG_EDIT_WORD_BE: size=2; bigendian=true; break;
				case HT_TAG_EDIT_DWORD_BE: size=4; bigendian=true; break;
				case HT_TAG_EDIT_QWORD_BE: size=8; bigendian=true; break;
			}
			
			int shift=4-(cursor_tag_micropos&1)*4;
			int m=~(0xf<<shift), o=nibval<<shift;

			int b;
			if (bigendian)  {
				b=(cursor_tag_micropos)/2;
			} else {
				b=size-(cursor_tag_micropos)/2-1;
			}
			
			byte buf;
			pread(cursor_tag_offset+b, &buf, 1);
			buf&=m;
			buf|=o;
			pwrite(cursor_tag_offset+b, &buf, 1);
			cursormicroedit_forward();
			return true;
		}
		case HT_TAG_EDIT_CHAR: {
			if ((b>=32) && (b<=0xff)) {
				pwrite(cursor_tag_offset, &b, 1);
				cursormicroedit_forward();
				return true;
			}
			break;
		}
		case HT_TAG_EDIT_BIT: {
			if ((b=='1') || (b=='0') || b==' ') {
				byte d;
				cursorline_get();
				char *t=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
				int shift=((ht_tag_edit_bit*)t)->bitidx;
				dword mask=1 << (shift%8);
				int op=shift/8;
				pread(cursor_tag_offset+op, &d, 1);
				switch (b) {
					case '0':
						d&=~mask;
						break;
					case '1':
						d|=mask;
						break;
					case K_Space:
						d^=mask;
						break;
				}
				pwrite(cursor_tag_offset+op, &d, 1);
				cursormicroedit_forward();
				return true;
			}
			break;
		}
		case HT_TAG_EDIT_TIME: {
			dword d;
			int h = edit_input_c2d(b);
					
			byte buf[4];
			if ((pread(cursor_tag_offset, &buf, 4)==4) && (h!=-1)) {
				d=(buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
				tm *t = gmtime((time_t*)&d);
				tm q = *t;
				int k;
				bool worked = false;
#define DEC_MASK(value, mask) ((value) - (value) / (mask) % 10 * (mask))
				switch (cursor_tag_micropos) {
					case 0:
						k = q.tm_hour % 10 + h * 10;
						if (k < 24) {
							q.tm_hour = k;
							worked = true;
						}
						break;
					case 1:
						k = q.tm_hour - q.tm_hour % 10 + h;
						if (k < 24) {
							q.tm_hour = k;
							worked = true;
						}
						break;
					case 2:
						k = q.tm_min % 10 + h * 10;
						if (k < 60) {
							q.tm_min = k;
							worked = true;
						}
						break;
					case 3:
						k = q.tm_min - q.tm_min % 10 + h;
						if (k < 60) {
							q.tm_min = k;
							worked = true;
						}
						break;
					case 4:
						k = q.tm_sec % 10 + h * 10;
						if (k < 60) {
							q.tm_sec = k;
							worked = true;
						}
						break;
					case 5:
						k = q.tm_sec - q.tm_sec % 10 + h;
						if (k < 60) {
							q.tm_sec = k;
							worked = true;
						}
						break;
					case 6:
						k = (q.tm_mday % 10) + h * 10;
						if (k <= 31) {
							q.tm_mday = k;
							worked = true;
						}
						break;
					case 7:
						k = q.tm_mday - q.tm_mday % 10 + h;
						if (k <= 31) {
							q.tm_mday = k;
							worked = true;
						}
						break;
					case 8:
						k = (q.tm_mon+1) % 10 + h * 10;
						if (k <= 12) {
							q.tm_mon = k-1;
							worked = true;
						}
						break;
					case 9:
						k = q.tm_mon - q.tm_mon % 10 + h;
						if (k <= 12) {
							q.tm_mon = k-1;
							worked = true;
						}
						break;
					case 10:
						k = DEC_MASK(q.tm_year, 1000) + h * 1000;
						if ((k+1900 >= 1970 ) && (k+1900 < 2106)) {
							q.tm_year = k;
							worked = true;
						}
						break;
					case 11:
						k = DEC_MASK(q.tm_year, 100) + h * 100;
						if ((k+1900 >= 1970 ) && (k+1900 < 2106)) {
							q.tm_year = k;
							worked = true;
						}
						break;
					case 12:
						k = DEC_MASK(q.tm_year, 10) + h * 10;
						if ((k+1900 >= 1970 ) && (k+1900 < 2106)) {
							q.tm_year = k;
							worked = true;
						}
						break;
					case 13:
						k = DEC_MASK(q.tm_year, 1) + h * 1;
						if ((k+1900 >= 1970 ) && (k+1900 < 2106)) {
							q.tm_year = k;
							worked = true;
						}
						break;
				}
				/* FIXME: big bad hack... */
				if (sizeof(dword) == sizeof(time_t))
				if (worked) {
				/* FIXME: !!! */
					dword tz;
					time_t l = time(NULL);
					time_t g = l;
					struct tm *gtm = gmtime(&g);
					g = mktime(gtm);
					tz = (dword)difftime(g, l);
					*(time_t*)&d = mktime(&q);
					d -= tz;
					buf[0] = d>>0;
					buf[1] = d>>8;
					buf[2] = d>>16;
					buf[3] = d>>24;
					pwrite(cursor_tag_offset, &buf, 4);
					cursormicroedit_forward();
					return true;
				}
			}
		}
	}
	return false;
}

int ht_uformat_viewer::edit_input_c2h(byte b)
{
	int h=-1;
	if ((b>='0') && (b<='9')) {
		h=b-'0';
	} else if ((b>='a') && (b<='f')) {
		h=b-'a'+10;
	} else if ((b>='A') && (b<='F')) {
		h=b-'A'+10;
	}
	return h;
}

int ht_uformat_viewer::edit_input_c2d(byte b)
{
	int h=-1;
	if ((b>='0') && (b<='9')) {
		h=b-'0';
	}
	return h;
}

void ht_uformat_viewer::edit_input_correctpos()
{
	ht_sub *sub;
	ID id1, id2;

/* try finding edited tag by its offset */
	sub=top_sub;
	id1=top_id1;
	id2=top_id2;
	char c_line[1024];  /* FIXME: possible buffer overflow ! */
	int g=cursor_tag_group;
	for (int y=0; y<size.h+10; y++) {
		if (!sub->getline(c_line, id1, id2)) break;
		int c=tag_count_selectable_tags_in_group(c_line, cursor_tag_group);
		for (int i=0; i<c; i++) {
			char *t=tag_get_selectable_tag(c_line, i, cursor_tag_group);
			if ((tag_get_class(t)==tag_class_edit) && (tag_get_offset(t)==cursor_tag_offset)) {
				set_cursor(sub, id1, id2);
				cursorline_dirty();
				cursor_tag_idx=i;
				cursor_tag_group=g;
				update_misc_info();
				update_visual_info();
				return;
			}
		}
		if (!next_line(&sub, &id1, &id2, 1)) break;
	}

/* try finding edited tag by cursor_ypos */
	sub=top_sub;
	id1=top_id1;
	id2=top_id2;
	next_line(&sub, &id1, &id2, cursor_ypos);
	int ci=cursor_tag_idx;
	set_cursor(sub, id1, id2);
	cursorline_dirty();
	cursorline_get();
	int x=tag_count_selectable_tags(cursor_line);
	if (ci>=x) ci=x-1;
	cursor_tag_idx=ci;
	update_misc_info();
	update_visual_info();
}

bool ht_uformat_viewer::edit_start()
{
	if (edit() && cursor_sub) {
		cursor_tag_micropos=0;
		cursorline_dirty();
		adjust_cursor_idx();
		update_visual_info();
		update_misc_info();
		dirtyview();
		return true;
	}
	return false;
}

bool ht_uformat_viewer::edit_update()
{
	if (edit()) {
		file->cntl(FCNTL_MODS_FLUSH);
		dirtyview();
		return true;
	}
	return false;
}

int ht_uformat_viewer::find_first_tag(ht_sub **_sub, ID *_id1, ID *_id2, int limit)
{
	char line[1024];	/* FIXME: possible buffer overflow ! */
	int i=0;
	ht_sub *sub=(*_sub);
	if (!sub) return 0;
	ID id1=*_id1, id2=*_id2;
	do {
		sub->getline(line, id1, id2);
		if (tag_get_selectable_tag(line, 0, 0)) {
			*_sub=sub;
			*_id1=id1;
			*_id2=id2;
			return 1;
		}
	} while ((next_line(&sub, &id1, &id2, 1)) && (i++<limit));
	return 0;
}

int ht_uformat_viewer::find_first_edit_tag_with_offset(ht_sub **_sub, ID *_id1, ID *_id2, int *tag_idx, int limit, FILEOFS offset)
{
	char line[1024];	/* FIXME: possible buffer overflow ! */
	int i=0;
	ht_sub *sub=(*_sub);
	ID id1=*_id1, id2=*_id2;
	do {
		sub->getline(line, id1, id2);
		int c=tag_count_selectable_tags(line);
		char *t=line;
		for (int j=0; j<c; j++) {
			t=tag_get_selectable_tag(t, 0, -1);
			if ((tag_get_class(t)==tag_class_edit) && (tag_get_offset(t)==offset)) {
				*tag_idx=j;	/* FIXME: what about groups ??? */
				*_sub=sub;
				*_id1=id1;
				*_id2=id2;
				return 1;
			}
			t+=tag_get_len(t);
		}
	} while ((next_line(&sub, &id1, &id2, 1)) && (i++<limit));
	return 0;
}

void ht_uformat_viewer::focus_cursor()
{
// why ?
/*	if (cursor_state==cursor_state_invisible) {
		center_view(cursor_sub, cursor_id1, cursor_id2);
		cursor_state=cursor_state_visible;
	}*/
	update_visual_info();
	if (cursor_visual_xpos-xscroll <= 1) {
		if (cursor_visual_xpos>=1) xscroll = cursor_visual_xpos-1; else
			xscroll = 0;
	} else if (cursor_visual_xpos+cursor_visual_length-xscroll > size.w-1) {
		xscroll = cursor_visual_xpos+cursor_visual_length-size.w+1;
	}
}

char *ht_uformat_viewer::func(UINT i, bool execute)
{
	switch (i) {
		case 3:
			if (caps & VC_REPLACE) {
				if (edit()) {
					if (execute) sendmsg(cmd_file_replace);
					return "replace";
				} else {
					return "~replace";
				}
			}
			break;
		case 4:
			if (caps & VC_EDIT) {
				if (edit()) {
					if (execute) baseview->sendmsg(cmd_view_mode_i, file, NULL);
					return "view";
				} else {
					if (execute) baseview->sendmsg(cmd_edit_mode_i, file, NULL);
					return "edit";
				}
			}
			break;
		case 5:
			if (caps & VC_GOTO) {
				if (execute) sendmsg(cmd_file_goto);
				return "goto";
			}
			break;
		case 7:
			if (caps & VC_SEARCH) {
				if (execute) sendmsg(cmd_file_search);
				return "search";
			}
			break;
		case 8:
			if (caps & VC_RESIZE) {
				if (edit()) {
					if (execute) sendmsg(cmd_file_resize);
					return "resize";
				} else {
					return "~resize";
				}
			}
			break;
		case 9:
			if (caps & VC_EDIT) {
				if (edit()) {
					if (execute) edit_update();
					return "update";
				} else {
					return "~update";
				}
			}
			break;
	}
	return ht_format_viewer::func(i, execute);
}

vcp ht_uformat_viewer::getcolor_tag(UINT pal_index)
{
	return getcolorv(&tagpal, pal_index);
}

bool ht_uformat_viewer::get_current_address(fmt_vaddress *addr)
{
	if (cursor_sub) {
		if (cursor_sub->convert_id_to_addr(cursor_id1, cursor_id2, addr)) return 1;
		*addr=cursor_id1;   // FIXME: hack, not ok !
		return 1;
	}
	return 0;
}

bool ht_uformat_viewer::get_current_offset(FILEOFS *offset)
{
	if ((cursor_state!=cursor_state_disabled) && (cursor_tag_class==tag_class_edit)) {
		*offset=cursor_tag_offset;
		return 1;
	}
	return 0;
}

int ht_uformat_viewer::get_current_tag(char **tag)
{
	cursorline_get();
	char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	if (e) {
		*tag=e;
		return 1;
	}
	return 0;
}

int ht_uformat_viewer::get_current_tag_size(dword *size)
{
	if (cursor_tag_class==tag_class_edit) {
		cursorline_get();
		char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
		if (e) {
			*size=tag_get_size(e);
			return 1;
		}
	}
	return 0;
}

void ht_uformat_viewer::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
		case msg_get_scrollinfo:
			switch (msg->data1.integer) {
				case gsi_pindicator: {
					get_pindicator_str((char*)msg->data2.ptr);
					break;
				}
				case gsi_hscrollbar: {
					gsi_scrollbar_t *p=(gsi_scrollbar_t*)msg->data2.ptr;
					if (!get_hscrollbar_pos(&p->pstart, &p->psize)) {
						p->pstart = 0;
						p->psize = 100;
					}
					break;
				}
				case gsi_vscrollbar: {
					gsi_scrollbar_t *p=(gsi_scrollbar_t*)msg->data2.ptr;
					if (!get_vscrollbar_pos(&p->pstart, &p->psize)) {
						p->pstart = 0;
						p->psize = 100;
					}
					break;
				}
			}
			clearmsg(msg);
			return;
		case msg_complete_init:
			complete_init();
			clearmsg(msg);
			return;
		case msg_keypressed:
			switch (msg->data1.integer) {
				case K_Alt_S:
					if (cursor_select) {
					    select_mode_off();
					} else {
					    select_mode_on();
					}
					clearmsg(msg);
					dirtyview();
					return;
				case K_Up:
					select_mode_pre();
					cursor_up(1);
					select_mode_post(0);
					clearmsg(msg);
					dirtyview();
					return;
				case K_Down:
					select_mode_pre();
					cursor_down(1);
					select_mode_post(0);
					clearmsg(msg);
					dirtyview();
					return;
				case K_PageUp:
					select_mode_pre();
					cursor_up(size.h);
					select_mode_post(0);
					clearmsg(msg);
					dirtyview();
					return;
				case K_PageDown:
					select_mode_pre();
					cursor_down(size.h);
					select_mode_post(0);
					clearmsg(msg);
					dirtyview();
					return;
				case K_Left:
					if (cursor_state!=cursor_state_disabled) {
						select_mode_pre();
						if (edit()) cursormicro_backward(); else cursor_left();
						select_mode_post(0);
						focus_cursor();
						clearmsg(msg);
						dirtyview();
						return;
					}
					break;
				case K_Right:
					if (cursor_state!=cursor_state_disabled) {
						int r;
						select_mode_pre();
						if (edit()) r=cursormicro_forward(); else r=cursor_right();
						select_mode_post(!r);
						focus_cursor();
						clearmsg(msg);
						dirtyview();
						return;
					}
					break;
				case K_Return:
					switch (cursor_tag_class) {
						case tag_class_sel:
							ref();
							break;
						case tag_class_edit:
							if (edit()) {
								edit_input(msg->data1.integer);
							}
							break;
					}
					dirtyview();
					clearmsg(msg);
					return;
				case K_BackSpace:
					if (edit()) {
						FILEOFS f;
						if (get_current_offset(&f)) {
							file->cntl(FCNTL_MODS_CLEAR_DIRTY_RANGE, f, 1);
							cursor_left();
							focus_cursor();
						}
					} else {
						pop_vs_history();
					}                         
					dirtyview();
					clearmsg(msg);
					return;
				case K_Tab: {
					int c=cursor_tag_group;
					cursor_tab();
					if (cursor_tag_group!=c) {
						focus_cursor();
						update_visual_info();
						dirtyview();
						clearmsg(msg);
						return;
					}
					break;
				}
				case K_Home:
					if (cursor_state==cursor_state_visible) {
						select_mode_pre();
						cursor_home();
						select_mode_post(0);
						focus_cursor();
						clearmsg(msg);
						dirtyview();
						return;
					}
					break;
				case K_End:
					if (cursor_state==cursor_state_visible) {
						select_mode_pre();
						cursor_end();
						select_mode_post(0);
						focus_cursor();
						clearmsg(msg);
						dirtyview();
						return;
					}
					break;
				case K_Control_Left:
					xscroll-=2;
					if (xscroll<0) xscroll=0;
					dirtyview();
					clearmsg(msg);
					return;
				case K_Control_Right:
					xscroll+=2;
					dirtyview();
					clearmsg(msg);
					return;
				case K_Control_PageUp: {
					top_sub=first_sub;
					if (top_sub) {
						push_vs_history(this);
						select_mode_pre();
						top_sub->first_line_id(&top_id1, &top_id2);

						ht_sub *sub=top_sub;
						ID id1=top_id1, id2=top_id2;
						if (find_first_tag(&sub, &id1, &id2, size.h)) {
							set_cursor(sub, id1, id2);
						} else {
							cursor_ypos=0x7fffffff;
							update_misc_info();
							update_visual_info();
							check_cursor_visibility();
						}
						select_mode_post(0);
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Control_PageDown: {
					top_sub=last_sub;
					if (top_sub) {
						push_vs_history(this);
						select_mode_pre();
						top_sub->last_line_id(&top_id1, &top_id2);

						ht_sub *sub=top_sub;
						ID id1=top_id1, id2=top_id2;
						if (find_first_tag(&sub, &id1, &id2, 1)) {
							set_cursor(sub, id1, id2);
						} else {
							cursor_ypos=0x7fffffff;
							update_misc_info();
							update_visual_info();
							check_cursor_visibility();
						}
						
						cursor_up(size.h);
						cursor_down(size.h);
						
						select_mode_post(0);
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_Up: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					cursor_up(1);
					if (s!=0xffffffff) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_Down: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					cursor_down(1);
					if (s!=0xffffffff) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_Left: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					int r;
					if (edit()) r=cursormicro_backward(); else r=cursor_left();
					if ((s!=0xffffffff) & (r)) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_Right: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					int r;
					if (edit()) r=cursormicro_forward(); else r=cursor_right();
					if (s!=0xffffffff) {
						if (r) {
							if (cursor_tag_class==tag_class_edit) {
								pselect_add(s, cursor_tag_offset);
							} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
								pselect_add(s, e);
							}
						} else {
							if ((cursor_tag_class==tag_class_edit) && (sel_end != e)) {
								pselect_add(s, e);
							}
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_PageUp: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					cursor_up(size.h);
					if (s!=0xffffffff) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_PageDown: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					cursor_down(size.h);
					if (s!=0xffffffff) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_Home: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					int r=cursor_home();
					if ((s!=0xffffffff) && (r)) {
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(cursor_tag_offset, s);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(e, s);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_End: {
					focus_cursor();
					FILEOFS s, e;
					dword ts;
					if (cursor_tag_class==tag_class_edit)
						s=cursor_tag_offset;
					else
						s=(sel_end>sel_start) ? sel_end : 0xffffffff;
					e=get_current_tag_size(&ts) ? s+ts : 0xffffffff;
					int r=cursor_end();
					if ((s!=0xffffffff) && (r)){
						if (cursor_tag_class==tag_class_edit) {
							pselect_add(s, cursor_tag_offset);
						} else if ((cursor_tag_class==tag_class_sel) && (e!=0xffffffff)) {
							pselect_add(s, e);
						}
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				case K_Shift_F7:
					if (!continue_search()) infobox("no further matches");
					dirtyview();
					clearmsg(msg);
					return;
				case K_Alt_C:
				case K_Control_Insert:
					sendmsg(cmd_edit_copy);
					dirtyview();
					clearmsg(msg);
					return;
				case K_Alt_V:
				case K_Shift_Insert:
					sendmsg(cmd_edit_paste);
					dirtyview();
					clearmsg(msg);
					return;
				case K_Escape:
					if (caps & VC_EDIT) {
						baseview->sendmsg(cmd_view_mode_i, file, NULL);
						return;
					}						
					break;
				default: {
					if (((dword)msg->data1.integer<=255) && (edit())) {
						if (cursor_tag_class==tag_class_edit) {
							focus_cursor();
							if (edit_input(msg->data1.integer)) {
								dirtyview();
								clearmsg(msg);
								return;
							}
						}
					}
				}
			}
			break;
		case cmd_edit_copy:
			if (sel_end>sel_start) {
				char dsc[1024]; /* FIXME: possible buffer overflow */
				sprintf(dsc, "%s::%s", file->get_filename(), desc);
				clipboard_copy(dsc, file, sel_start, sel_end-sel_start);
			}
			clearmsg(msg);
			return;
		case cmd_edit_paste:
			FILEOFS ofs;
			if (get_current_offset(&ofs)) {
				baseview->sendmsg(cmd_edit_mode_i, file, NULL);
				if (file->get_access_mode() & FAM_WRITE) {
					dword s=clipboard_paste(file, ofs);
					if (s) {
						pselect_set(ofs, ofs+s);
						sendmsg(cmd_edit_mode_i);
					}
				}
			}
			dirtyview();
			clearmsg(msg);
			return;
		case cmd_edit_mode:
			edit_start();
			dirtyview();
			return;
		case cmd_view_mode:
			edit_end();
			dirtyview();
			return;
		case cmd_file_goto: {
			char addrstr[1024];
			addrstr[0] = 0;
			if (inputbox("goto", "~address", addrstr, 1024, HISTATOM_GOTO)) {
				fmt_vaddress addr;
				globalerror[0]=0;
				if (!string_to_address(addrstr, &addr) || !goto_address(addr, this)) {
					if (globalerror[0]) infobox("error: %s\nin '%s'", globalerror, addrstr);
					    else infobox("address '%s' (=0x%x) not found !", addrstr, addr);
				}
			}
			clearmsg(msg);
			dirtyview();
			return;
		}
		case cmd_file_search: {
			ht_search_request *request = search_dialog(this, search_caps);
			ht_search_result *result = NULL;
			if (request) {
				switch (request->search_class) {
					case SC_PHYSICAL: {
						FILEOFS start=0, end=0xffffffff;
						get_current_offset(&start);
						result = psearch(request, start, end);
						break;
					}
					case SC_VISUAL: {
						fmt_vaddress start=0, end=0xffffffff;
						get_current_address(&start);
						result = vsearch(request, start, end);
						break;
					}
				}
				if (result) {
					// FIXME: !!!!!!!!
					if (!show_search_result(result)) infobox("couldn't display result (internal error)");
					delete result;
				} else infobox("not found");
			}

			clearmsg(msg);
			dirtyview();
			return;
		}
		case cmd_file_replace: {
			UINT s = get_file()->get_size();
			replace_dialog(this, search_caps);
			if (s != get_file()->get_size()) {
				sendmsg(msg_filesize_changed);
			}
			clearmsg(msg);
			dirtyview();
			return;
		}
		case msg_filesize_changed: {
			htmsg m;
			m.msg=msg_filesize_changed;
			m.type=mt_broadcast;
			sendsubmsg(&m);
			break;
		}
		case cmd_file_save: {
			if (edit()) edit_update();
			clearmsg(msg);
			return;
		}
		case cmd_file_resize: {
			FILEOFS o=0;
			if (get_current_offset(&o)) {
				dword s;
				get_current_tag_size(&s);
				o+=s;
			}
			
			UINT s=file->get_size();

			char buf[32];
			sprintf(buf, "%d", o);
			if (inputbox("resize", "new file size", buf, sizeof buf, 0)==button_ok) {
				scalar_t r;
				if (eval(&r, buf, NULL, NULL, NULL)) {
					int_t i;
					scalar_context_int(&r, &i);
					scalar_destroy(&r);
					o=i.value;
					if (o<s) {
					/* truncate */
						htmsg m;
			
						m.msg=cmd_file_truncate;
						m.type=mt_broadcast;
						m.data1.ptr=file;
						m.data2.integer=o;
						baseview->sendmsg(&m);

						m.msg=msg_filesize_changed;
						m.type=mt_broadcast;
						sendsubmsg(&m);
						sendmsg(&m);
					} else if (o>s) {
					/* extend */
						htmsg m;
			
						m.msg=cmd_file_extend;
						m.type=mt_broadcast;
						m.data1.ptr=file;
						m.data2.integer=o;
						baseview->sendmsg(&m);

						m.msg=msg_filesize_changed;
						m.type=mt_broadcast;
						sendmsg(&m);
					}
				} else {
					char *errmsg="?";
					int errpos=0;
					get_eval_error(&errmsg, &errpos);
					errorbox("eval error: %s at %d\n in '%s'", errmsg, errpos, buf);
				}
				update_misc_info();
			}

			clearmsg(msg);
			dirtyview();
			return;
		}
		case cmd_file_blockop: {
			if (sel_end>sel_start) {
				blockop_dialog(this, sel_start, sel_end);
			} else {
				FILEOFS o=0;
				get_current_offset(&o);
				blockop_dialog(this, o, file->get_size());
			}
			clearmsg(msg);
			dirtyview();
			return;
		}
	}
	ht_format_viewer::handlemsg(msg);
}

void ht_uformat_viewer::insertsub(ht_sub *sub)
{
	if (last_sub) last_sub->next=sub;
	sub->uformat_viewer=this;
	sub->prev=last_sub;
	last_sub=sub;
	if (!first_sub) first_sub=sub;
}

bool ht_uformat_viewer::goto_offset(FILEOFS offset, ht_view *source_object)
{
	ID id1, id2;
	ht_sub *sub=first_sub;
	while (sub) {
		if (sub->convert_ofs_to_id(offset, &id1, &id2)) {
			int tag_idx;
			if (source_object) push_vs_history(source_object);
			if (find_first_edit_tag_with_offset(&sub, &id1, &id2, &tag_idx, size.h, offset)) {
				set_cursor_ex(sub, id1, id2, tag_idx, 0);
				return 1;
			} else if (find_first_tag(&sub, &id1, &id2, size.h)) {
				set_cursor_ex(sub, id1, id2, 0, 0);
				return 1;
			}
			break;
		}
		sub=sub->next;
	}
	return 0;
}

bool ht_uformat_viewer::goto_address(fmt_vaddress addr, ht_view *source_object)
{
	ID id1, id2;
	ht_sub *sub=first_sub;
	FILEOFS offset;
	bool have_offset=address_to_offset(addr, &offset);
	while (sub) {
		if (sub->convert_addr_to_id(addr, &id1, &id2)) {
			int tag_idx;
			if (source_object) push_vs_history(source_object);
			if (have_offset && find_first_edit_tag_with_offset(&sub, &id1, &id2, &tag_idx, size.h, offset)) {
				set_cursor_ex(sub, id1, id2, tag_idx, 0);
				return 1;
			} else if (find_first_tag(&sub, &id1, &id2, size.h)) {
				set_cursor_ex(sub, id1, id2, 0, 0);
				return 1;
			}
			break;
		}
		sub=sub->next;
	}
	return 0;
}

bool ht_uformat_viewer::next_logical_address(fmt_vaddress addr, fmt_vaddress *naddr)
{
// FIXME: hack: we assume addr lies inside the current sub
	ht_sub *sub=cursor_sub;
	ID id1, id2;
	if (sub->convert_addr_to_id(addr, &id1, &id2) &&
	next_line(&sub, &id1, &id2, 1)) {
		return sub->convert_id_to_addr(id1, id2, naddr);
	}
	return 0;
}

bool ht_uformat_viewer::next_logical_offset(FILEOFS ofs, FILEOFS *nofs)
{
// FIXME: hack: we assume ofs lies inside the current sub
	ht_sub *sub=cursor_sub;
	ID id1, id2;
	if (sub->convert_ofs_to_id(ofs, &id1, &id2) &&
	next_line(&sub, &id1, &id2, 1)) {
		return sub->convert_id_to_ofs(id1, id2, nofs);
	}
	return 0;
}

int ht_uformat_viewer::next_line(ht_sub **sub, ID *id1, ID *id2, int n)
{
	int in=n;
	while (n) {
		if (n-=(*sub)->next_line_id(id1, id2, n)) {
			if (!(*sub)->next) return in-n;
			*sub=(*sub)->next;
			(*sub)->first_line_id(id1, id2);
			n--;
		}
	}
	return in-n;
}

int ht_uformat_viewer::prev_line(ht_sub **sub, ID *id1, ID *id2, int n)
{
	int in=n;
	while (n){
		if (n-=(*sub)->prev_line_id(id1, id2, n)) {
			if (!(*sub)->prev) return in-n;
			*sub=(*sub)->prev;
			(*sub)->last_line_id(id1, id2);
			n--;
		}
	}
	return in-n;
}

vcp ht_uformat_viewer::get_tag_color_edit(FILEOFS tag_offset, UINT size, bool atcursoroffset, bool iscursor)
{
	vcp tag_color = getcolor_tag(palidx_tags_edit_tag);
	bool isdirty = false;
	file->cntl(FCNTL_MODS_IS_DIRTY, tag_offset, size, &isdirty);
	if (isdirty) tag_color=vcp_mix(tag_color, getcolor_tag(palidx_tags_edit_tag_modified));
	if ((tag_offset>=sel_start) && (tag_offset<sel_end)) tag_color=vcp_mix(tag_color, getcolor_tag(palidx_tags_edit_tag_selected));
	if (iscursor) {
		int coloridx;
		if (atcursoroffset) {
			if (edit()) {
				coloridx=palidx_tags_edit_tag_cursor_edit;
			} else {
				coloridx=palidx_tags_edit_tag_cursor_select;
			}
		} else {
			coloridx=palidx_tags_edit_tag_cursor_unfocused;
		}
		tag_color=vcp_mix(tag_color, getcolor_tag(coloridx));
	}
	return tag_color;
}

void ht_uformat_viewer::render_tagstring_desc(char **string, int *length, vcp *tc, char *tag, UINT size, bool bigendian, bool is_cursor)
{
	ID id;
	vcp tag_color=getcolor_tag(palidx_tags_sel_tag);
	if (is_cursor) tag_color=getcolor_tag(palidx_tags_sel_tag_cursor_focused);
	*string="?";
	*length=1;
	*tc=tag_color;
	if (tag_get_desc_id(tag, &id)) {
		int_hash *tbl;
		if ((tbl=(int_hash*)find_atom(id))) {
			char *str;
			qword q;
			q.hi = 0;
			q.lo = 0;
			FILEOFS tag_offset=tag_get_offset(tag);
			byte buf[4];
			
			if (pread(tag_offset, buf, size)==size) {
				switch (size) {
					case 1:
						q.hi = 0;
						q.lo = buf[0];
						break;
					case 2:
						q.hi = 0;
						if (bigendian) {
							q.lo = (buf[0]<<8) | buf[1];
						} else {
							q.lo = (buf[1]<<8) | buf[0];
						}
						break;
					case 4:
						q.hi = 0;
						if (bigendian) {
							q.lo = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
						} else {
							q.lo = (buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
						}
						break;
					case 8:
						if (bigendian) {
							q.hi = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
							q.lo = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
						} else {
							q.hi = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
							q.lo = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
						}
						break;
				}
/* FIXME: qword ? */
				if ((str=matchhash(q.lo, tbl))) {
					*string=str;
					*length=strlen(*string);
				}
			} else {
				*string="?";
				*length=strlen(*string);
			}
		}
	}
}

void ht_uformat_viewer::reloadpalette()
{
	ht_format_viewer::reloadpalette();
	if (tagpal.data) {
	    free(tagpal.data);
	    tagpal.data = NULL;
	}	    
	load_pal(palclasskey_tags, palkey_tags_default, &tagpal);
}

UINT ht_uformat_viewer::render_tagstring(char *chars, vcp *colors, UINT maxlen, char *tagstring, bool cursor_in_line)
{
	char *n=tagstring;
	UINT c=0;
	int i=0, g=0;
	bool is_cursor;
	vcp color_normal=getcolor(palidx_generic_body);
	do {
		int l=0;
		while (n[l] && n[l]!='\e') { l++; }
		c+=render_tagstring_single(chars, colors, maxlen, c, n, l, color_normal);
		
		n+=l;
		is_cursor=cursor_in_line && (i==cursor_tag_idx);
		if (*n=='\e') {
			FILEOFS tag_offset;
			vcp tag_color;
			char str[32];
			switch (n[1]) {
				case HT_TAG_EDIT_BYTE: {
					byte d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 1, focused && (g==cursor_tag_group), is_cursor);

					if (pread(tag_offset, &d, 1)==1) {
						mkhexb(str, d);
					} else {
						strcpy(str, "??");
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 2, tag_color);
		
					n+=HT_TAG_EDIT_BYTE_LEN;
					break;
				}
				case HT_TAG_EDIT_WORD_LE: {
					word d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 2, (g==cursor_tag_group), is_cursor);
					
					byte buf[2];
					if (pread(tag_offset, &buf, 2)==2) {
						/* little endian */
						d=(buf[1] << 8) | buf[0];
						mkhexw(str, d);
					} else {
						strcpy(str, "????");
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 4, tag_color);
					n+=HT_TAG_EDIT_WORD_LE_LEN;
					break;
				}
				case HT_TAG_EDIT_DWORD_LE: {
					dword d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 4, (g==cursor_tag_group), is_cursor);
					
					byte buf[4];
					if (pread(tag_offset, &buf, 4)==4) {
						/* little endian */
						d=(buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
						mkhexd(str, d);
					} else {
						strcpy(str, "????????");
					}

					c+=render_tagstring_single(chars, colors, maxlen, c, str, 8, tag_color);
					n+=HT_TAG_EDIT_DWORD_LE_LEN;
					break;
				}
				case HT_TAG_EDIT_QWORD_LE: {
					qword q;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 8, (g==cursor_tag_group), is_cursor);
					
					byte buf[8];
					if (pread(tag_offset, &buf, 8)==8) {
						/* little endian */
						q.hi = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
						q.lo = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
						mkhexq(str, q);
					} else {
						strcpy(str, "????????????????");
					}

					c+=render_tagstring_single(chars, colors, maxlen, c, str, 16, tag_color);
					n+=HT_TAG_EDIT_QWORD_LE_LEN;
					break;
				}
				case HT_TAG_EDIT_WORD_BE: {
					word d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 2, (g==cursor_tag_group), is_cursor);
					
					byte buf[2];
					if (pread(tag_offset, &buf, 2)==2) {
						/* big endian */
						d=(buf[0] << 8) | buf[1];
						mkhexw(str, d);
					} else {
						strcpy(str, "????");
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 4, tag_color);
					n+=HT_TAG_EDIT_WORD_BE_LEN;
					break;
				}
				case HT_TAG_EDIT_DWORD_BE: {
					dword d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 4, (g==cursor_tag_group), is_cursor);
					
					byte buf[4];
					if (pread(tag_offset, &buf, 4)==4) {
						/* big endian */
						d=(buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
						mkhexd(str, d);
					} else {
						strcpy(str, "????????");
					}
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 8, tag_color);
					n+=HT_TAG_EDIT_DWORD_BE_LEN;
					break;
				}
				case HT_TAG_EDIT_QWORD_BE: {
					qword q;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 8, (g==cursor_tag_group), is_cursor);
					
					byte buf[8];
					if (pread(tag_offset, &buf, 8)==8) {
						/* big endian */
						q.hi = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
						q.lo = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
						mkhexq(str, q);
					} else {
						strcpy(str, "????????????????");
					}
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 16, tag_color);
					n+=HT_TAG_EDIT_QWORD_BE_LEN;
					break;
				}
				case HT_TAG_EDIT_TIME: {
					dword d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 4, (g==cursor_tag_group), is_cursor);

					byte buf[4];
					if (pread(tag_offset, &buf, 4)==4) {
						d=(buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
						tm *t=gmtime((time_t*)&d);
						sprintf(str, "%02d:%02d:%02d %02d.%02d.%04d +1900", t->tm_hour, t->tm_min, t->tm_sec, t->tm_mday, t->tm_mon+1, t->tm_year);
					} else {
						strcpy(str, "?");
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, str, strlen(str), tag_color);
					n+=HT_TAG_EDIT_TIME_LEN;
					break;
				}
				case HT_TAG_EDIT_CHAR: {
					char d;
					
					tag_offset=tag_get_offset(n);
					tag_color=get_tag_color_edit(tag_offset, 1, (g==cursor_tag_group), is_cursor);
					
					if (pread(tag_offset, &d, 1)!=1) {
						d='?';
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, &d, 1, tag_color);
					n+=HT_TAG_EDIT_CHAR_LEN;
					break;
				}
				case HT_TAG_EDIT_BIT: {
					int shift=((ht_tag_edit_bit*)n)->bitidx;
					int op=shift/8;
					byte d;
					
					tag_offset=tag_get_offset(n);
					tag_color=getcolor_tag(palidx_tags_edit_tag);
					bool isdirty = false;
					file->cntl(FCNTL_MODS_IS_DIRTY, tag_offset+op, IS_DIRTY_SINGLEBIT | (shift & 7), &isdirty);
					if (isdirty) tag_color=vcp_mix(tag_color, getcolor_tag(palidx_tags_edit_tag_modified));
					if ((tag_offset>=sel_start) && (tag_offset<sel_end)) tag_color=vcp_mix(tag_color, getcolor_tag(palidx_tags_edit_tag_selected));
					if (is_cursor) tag_color=vcp_mix(tag_color, getcolor_tag(edit() ? palidx_tags_edit_tag_cursor_edit : palidx_tags_edit_tag_cursor_select));
					
					if (pread(tag_offset+op, &d, 1)==1) {
						str[0]=(d& (1 << (shift%8))) ? '1' : '0';
						str[1]=0;
					} else {
						strcpy(str, "?");
					}
					
					c+=render_tagstring_single(chars, colors, maxlen, c, str, 1, tag_color);
					n+=HT_TAG_EDIT_BIT_LEN;
					break;
				}
				case HT_TAG_EDIT_SELVIS: {
					tag_offset=tag_get_offset(n);
					tag_color=getcolor_tag(palidx_tags_edit_tag);

					if ((tag_offset>=sel_start) && (tag_offset<sel_end)) tag_color=vcp_mix(tag_color, getcolor_tag(palidx_tags_edit_tag_selected));

					c+=render_tagstring_single(chars, colors, maxlen, c, &((ht_tag_edit_selvis*)n)->ch, 1, tag_color);
					n+=HT_TAG_EDIT_SELVIS_LEN;
					continue;
				}
				case HT_TAG_SEL: {
					int tag_textlen=tag_get_seltextlen(n);
					tag_color=getcolor_tag(palidx_tags_sel_tag);
					if (is_cursor) tag_color = getcolor_tag(
							(focused && (g==cursor_tag_group)) ?
							palidx_tags_sel_tag_cursor_focused :
							palidx_tags_sel_tag_cursor_unfocused
						);
					n+=HT_TAG_SEL_LEN(0);
					c+=render_tagstring_single(chars, colors, maxlen, c, n, tag_textlen, tag_color);
					n+=tag_textlen;
					break;
				}
				case HT_TAG_DESC_BYTE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 1, true, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_BYTE_LEN;
					break;
				}
				case HT_TAG_DESC_WORD_LE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 2, false, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_WORD_LE_LEN;
					break;
				}
				case HT_TAG_DESC_DWORD_LE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 4, false, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_DWORD_LE_LEN;
					break;
				}
				case HT_TAG_DESC_QWORD_LE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 8, false, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_QWORD_LE_LEN;
					break;
				}
				case HT_TAG_DESC_WORD_BE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 2, true, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_WORD_BE_LEN;
					break;
				}
				case HT_TAG_DESC_DWORD_BE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 4, true, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_DWORD_BE_LEN;
					break;
				}
				case HT_TAG_DESC_QWORD_BE: {
					char *str;
					int l;
					render_tagstring_desc(&str, &l, &tag_color, n, 8, true, is_cursor);
					c+=render_tagstring_single(chars, colors, maxlen, c, str, l, tag_color);
					n+=HT_TAG_DESC_DWORD_BE_LEN;
					break;
				}
				case HT_TAG_FLAGS:
					n+=HT_TAG_FLAGS_LEN;
					tag_color=getcolor_tag(palidx_tags_sel_tag);
					if (is_cursor) tag_color=getcolor_tag(palidx_tags_sel_tag_cursor_focused);
					c+=render_tagstring_single(chars, colors, maxlen, c, "details", 7, tag_color);
					break;
				case HT_TAG_GROUP:
					n+=HT_TAG_GROUP_LEN;
					g++;
					i=0;
					continue;
				case HT_TAG_COLOR:
					color_normal=((ht_tag_color*)n)->color;
					if (color_normal == (int)0xffffffff) {
						color_normal = getcolor(palidx_generic_body);
					}
					n+=HT_TAG_COLOR_LEN;
					continue;
				default: {
					assert(0);
				}
			}
		}
		i++;
	} while (*n);
	return c;
}

UINT ht_uformat_viewer::render_tagstring_single(char *chars, vcp *colors, UINT maxlen, UINT offset, char *text, UINT len, vcp color)
{
	if (chars) chars+=offset;
	if (colors) colors+=offset;
	maxlen-=offset;
	UINT l=(len<maxlen) ? len : maxlen, r=0;
	while (l--) {
		if (chars) *(chars++)=*(text++);
		if (colors) *(colors++)=color;
		r++;
	}
	return r;
}

#define MAX_PRINT_TAGSTRING_LINELENGTH 256

void ht_uformat_viewer::print_tagstring(int x, int y, int maxlen, int xscroll, char *tagstring, bool cursor_in_line)
{
	char text[MAX_PRINT_TAGSTRING_LINELENGTH], *t=text+xscroll;
	vcp color[MAX_PRINT_TAGSTRING_LINELENGTH], *c=color+xscroll;
	int l=render_tagstring(text, color,
		(maxlen+xscroll+1>MAX_PRINT_TAGSTRING_LINELENGTH) ? MAX_PRINT_TAGSTRING_LINELENGTH
		: maxlen+xscroll+1, tagstring, cursor_in_line);

	if (l>xscroll) {
		l-=xscroll;
		while (l--) {
			if (x>=size.w) {
				buf_printchar(x-1, y, VCP(VC_GREEN, VC_TRANSPARENT), '>');
				break;
			}
			buf_printchar(x, y, *c, (unsigned char)*t);
			t++;
			c++;
			x++;
		}
	}
}

void ht_uformat_viewer::select_mode_off()
{
	if (cursor_select) {
		cursor_select=0;
	}
}

void ht_uformat_viewer::select_mode_on()
{
	if (!cursor_select) {
		cursor_select=1;
	}
}

void ht_uformat_viewer::select_mode_pre()
{
	if (cursor_select) {
		if (cursor_tag_class==tag_class_edit)
			cursor_select_start=cursor_tag_offset;
		else
			cursor_select_start=(sel_end>sel_start) ? sel_end : 0xffffffff;
		if (!get_current_tag_size(&cursor_select_cursor_length)) cursor_select_cursor_length=0xffffffff;
	}		
}	
	
void ht_uformat_viewer::select_mode_post(bool lastpos)
{
	if (cursor_select) {
		if (cursor_select_start!=0xffffffff) {
			if (cursor_tag_class==tag_class_edit) {
				if ((lastpos) && (cursor_select_cursor_length!=0xffffffff)) {
					FILEOFS s, e;
					pselect_get(&s, &e);
					if (e != cursor_tag_offset+cursor_select_cursor_length) {
						pselect_add(cursor_select_start, cursor_tag_offset+cursor_select_cursor_length);
					}
				} else {
					pselect_add(cursor_select_start, cursor_tag_offset);
				}
			} else if ((cursor_tag_class==tag_class_sel) && (cursor_select_cursor_length!=0xffffffff)) {
				pselect_add(cursor_select_start, cursor_select_start+cursor_select_cursor_length);
			}
		}
	}
}

UINT ht_uformat_viewer::pwrite(FILEOFS ofs, void *buf, UINT size)
{
	cursorline_dirty();
	return ht_format_viewer::pwrite(ofs, buf, size);
}

int ht_uformat_viewer::ref()
{
	cursorline_get();
	char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	if (!e) return 0;
	if (tag_get_class(e)==tag_class_sel) {
		if (!cursor_sub->ref(cursor_tag_id.low, cursor_tag_id.high)) {
			switch (e[1]) {
				case HT_TAG_SEL:
					return ref_sel(cursor_tag_id.low, cursor_tag_id.high);
				case HT_TAG_FLAGS:
					return ref_flags(((ht_tag_flags*)e)->offset, ((ht_tag_flags*)e)->id);
				case HT_TAG_DESC_BYTE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 1, true);
				case HT_TAG_DESC_WORD_LE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 2, false);
				case HT_TAG_DESC_DWORD_LE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 4, false);
				case HT_TAG_DESC_QWORD_LE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 8, false);
				case HT_TAG_DESC_WORD_BE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 2, true);
				case HT_TAG_DESC_DWORD_BE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 4, true);
				case HT_TAG_DESC_QWORD_BE:
					return ref_desc(((ht_tag_desc_byte*)e)->id, ((ht_tag_desc_byte*)e)->offset, 8, true);
			}
		}
	}
	return 0;
}

int ht_uformat_viewer::ref_desc(ID id, FILEOFS offset, UINT size, bool bigendian)
{
	int_hash *desc=(int_hash*)find_atom(id);
	if (desc) {
		bounds b;
		b.w=60;
		b.h=14;
		b.x=(screen->size.w-b.w)/2;
		b.y=(screen->size.h-b.h)/2;
		ht_dialog *g=new ht_dialog();
		g->init(&b, "desc", FS_KILLER | FS_MOVE);

		b.x=0;
		b.y=0;
		b.w-=2;
		b.h-=2;
		ht_itext_listbox *l=new ht_itext_listbox();
		l->init(&b, 2, 1);

		byte buf[4];
		int curpos=0, i=0;
		int d=0;

		if (pread(offset, buf, size)!=size) return 0;
		
		switch (size) {
			case 1: d=buf[0]; break;
			case 2:
				if (bigendian) {
					d=(buf[0]<<8) | buf[1];
				} else {
					d=(buf[1]<<8) | buf[0];
				}
				break;
			case 4:
				if (bigendian) {
					d=(buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
				} else {
					d=(buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
				}
				break;
		}

		int_hash *dsc=desc;
		while (dsc->desc) {
			char buf[1024]; /* FIXME: possible buffer overflow */
			switch (size) {
				case 1:
					sprintf(buf, "0x%02x", dsc->value);
					break;
				case 2:
					sprintf(buf, "0x%04x", dsc->value);
					break;
				case 4:
					sprintf(buf, "0x%08x", dsc->value);
					break;
			}
			if (dsc->value==d) {
				curpos=i;
			}
			l->insert_str(i, buf, dsc->desc);
			dsc++;
			i++;
		}

		l->update();
		l->seek(curpos);

		g->insert(l);
		g->setpalette(palkey_generic_window_default);

		if (g->run(0)==button_ok) {
			ht_listbox_data da;
			l->databuf_get(&da);
			int i=da.cursor_id;
			if (desc[i].value != d) {
				baseview->sendmsg(cmd_edit_mode_i, file, NULL);
				if (edit()) {
					pwrite(offset, &desc[i].value, size);
					dirtyview();
				}					
			}
		}

		g->done();
		delete g;
		return 1;
	}
	return 0;
}

int ht_uformat_viewer::ref_flags(ID id, FILEOFS offset)
{
	ht_tag_flags_s *flags=(ht_tag_flags_s*)find_atom(id), *fl;
	if (flags) {
		bounds b;
		b.w=60;
		b.h=14;
		b.x=(screen->size.w-b.w)/2;
		b.y=(screen->size.h-b.h)/2;
		ht_dialog *d=new ht_dialog();
		d->init(&b, (flags->bitidx==-1) ? flags->desc : 0, FS_KILLER | FS_TITLE | FS_MOVE);

		b.x=0;
		b.y=0;
		b.w-=2;
		b.h-=2;
		ht_uformat_viewer *u=new ht_uformat_viewer();
		u->init(&b, 0, VC_EDIT, file, 0);

		ht_mask_sub *m=new ht_mask_sub();
		m->init(file, 0);
		char *t, x[256];

		int width=0;
		fl=flags;
		if (fl->bitidx==-1) fl++;
		do {
			int l=strlen(fl->desc);
			if (l>width) width=l;
			fl++;
		} while (fl->desc);

		width=MAX(width, 25);
		width++;

		fl=flags;
		if (fl->bitidx==-1) fl++;
		do {
			t=x;
			int l=strlen(fl->desc);
			memmove(t, fl->desc, l);
			t+=l;
			while (t<x+width) *(t++)=' ';
			t=tag_make_edit_bit(t, offset, fl->bitidx);
			*t=0;
			m->add_mask(x);
			fl++;
		} while (fl->desc);


		u->insertsub(m);
		u->sendmsg(msg_complete_init, 0);
		d->insert(u);

		d->setpalette(palkey_generic_window_default);

		UINT pmode=file->get_access_mode() & FAM_WRITE;

		if (d->run(0)==button_ok) u->edit_update();

		if (pmode != file->get_access_mode() & FAM_WRITE) {
			if (pmode) {
				baseview->sendmsg(cmd_view_mode_i, file, NULL);
			} else {
				baseview->sendmsg(cmd_edit_mode_i, file, NULL);
			}
		}

		d->done();
		delete d;
		return 1;
	}
	return 0;
}

int ht_uformat_viewer::ref_sel(ID id_low, ID id_high)
{
	return 0;
}

ht_search_result *ht_uformat_viewer::psearch(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	last_search_request=(ht_search_request*)search->duplicate();
	last_search_physical=1;
	last_search_end_ofs=end;
	
	ht_sub *sub=first_sub;
	while (sub) {
		ht_search_result *r=sub->search(search, start, end);
		if (r) return r;
		sub=sub->next;
	}
	return 0;
}

ht_search_result *ht_uformat_viewer::vsearch(ht_search_request *search, fmt_vaddress start, fmt_vaddress end)
{
	last_search_request=(ht_search_request*)search->duplicate();
	last_search_physical=0;
	last_search_end_addr=end;
	
	if ((search->search_class==SC_VISUAL) && (search->type==ST_REGEX)) {
		if (!cursor_sub) return 0;
		ht_regex_search_request *s=(ht_regex_search_request*)search;
		
		ht_sub *sub;
		ID id1, id2;
		fmt_vaddress caddr;
		if ((get_current_address(&caddr)) && (start==caddr)) {
			sub=cursor_sub;
			id1=cursor_id1;
			id2=cursor_id2;
		} else {
			sub=first_sub;
			while (sub) {
				if (sub->convert_addr_to_id(start, &id1, &id2)) break;
				sub=sub->next;
			}
			if (!sub) {
			// fallback to beginning
				sub=first_sub;
				sub->first_line_id(&id1, &id2);
			}
		}
/* build progress indicator */
		bounds b;
		get_std_progress_indicator_metrics(&b);
		ht_progress_indicator *progress_indicator=new ht_progress_indicator();
		progress_indicator->init(&b, "ESC to cancel");
		UINT lines=0;
		fmt_vaddress lowest_va=0, highest_va=0;
		first_sub->convert_id_to_addr(id1, id2, &lowest_va);
		ID lid1, lid2;
		last_sub->last_line_id(&lid1, &lid2);
		last_sub->convert_id_to_addr(lid1, lid2, &highest_va);
		
		while (sub) {
			do {
				char line[1024];	/* FIXME: possible buffer overflow ! */
				if (!sub->getline(line, id1, id2)) assert(0);
				char rdrd[256];
				int c=render_tagstring(rdrd, 0, 256, line, 0);
				rdrd[c]=0;
				regmatch_t pos;
				if (!regexec(&s->rx, rdrd, 1, &pos, 0)) {
					ht_visual_search_result *r=new ht_visual_search_result();
					sub->convert_id_to_addr(id1, id2, &r->address);
					r->xpos=pos.rm_so;
					r->length=pos.rm_eo-pos.rm_so;
					progress_indicator->done();
					delete progress_indicator;
					
					return r;
				}
				lines++;
				if (lines % 500==0) {
					if (ht_keypressed()) {
						if (ht_getkey()==K_Escape) goto leave;
					}

					fmt_vaddress va;
					sub->convert_id_to_addr(id1, id2, &va);

					char text[1024];	/* FIXME: possible buffer overflow */
					if (highest_va>lowest_va) {
						int p=100*(va-lowest_va)/(highest_va-lowest_va);
						sprintf(text, "searching for '%s'... %d%% complete (%d lines)", s->rx_str, p, lines);
					} else {
						sprintf(text, "searching for '%s'... ?%% complete (%d lines)", s->rx_str, lines);
					}
					progress_indicator->settext(text);
					progress_indicator->sendmsg(msg_draw, 0);
					screen->show();
				}
			} while (sub->next_line_id(&id1, &id2, 1));
			sub=sub->next;
			if (sub) sub->first_line_id(&id1, &id2);
		}
leave:
		progress_indicator->done();
		delete progress_indicator;
	}
	return 0;
}

void ht_uformat_viewer::pselect_add(FILEOFS start, FILEOFS end)
{
	bool downward = (start<end);
	if (end<start) {
		FILEOFS temp=start;
		start=end;
		end=temp;
	}
	if ((end==sel_start) && !downward) {
		sel_start=start;
	} else if ((start==sel_end) && downward) {
		sel_end=end;
	} else if ((end==sel_end) && !downward) {
		sel_end=start;
	} else if ((start==sel_start) && downward) {
		sel_start=end;
	} else {
		sel_start=start;
		sel_end=end;
	}
	if (sel_start>sel_end) {
		FILEOFS temp=sel_start;
		sel_start=sel_end;
		sel_end=temp;
	}
}

void ht_uformat_viewer::pselect_get(FILEOFS *start, FILEOFS *end)
{
	*start=sel_start;
	*end=sel_end;
}

void ht_uformat_viewer::pselect_set(FILEOFS start, FILEOFS end)
{
	sel_start=start;
	sel_end=end;
}

void ht_uformat_viewer::clear_subs()
{
	ht_sub *s=first_sub, *t;
	while (s) {
		t=s->next;
		s->done();
		delete s;
		s=t;
	}

	uf_initialized=false;
	cursor_ypos=0;

	top_sub=0;
	first_sub=0;
	last_sub=0;
	cursor_sub=0;
}

void ht_uformat_viewer::sendsubmsg(int msg)
{
	htmsg m;
	m.msg=msg;
	sendsubmsg(&m);
}

void ht_uformat_viewer::sendsubmsg(htmsg *msg)
{
	if (msg->type==mt_broadcast) {
		ht_sub *s=first_sub;
		while (s) {
			s->handlemsg(msg);
			s=s->next;
		}
	} else cursor_sub->handlemsg(msg);
}

int ht_uformat_viewer::set_cursor(ht_sub *sub, ID id1, ID id2)
{
	return set_cursor_ex(sub, id1, id2, -1, -1);
}

int ht_uformat_viewer::set_cursor_ex(ht_sub *sub, ID id1, ID id2, int tag_idx, int tag_group)
{
	cursorline_dirty();
	ht_sub *tsub=top_sub;
	ID tid1=top_id1, tid2=top_id2;
	int ty=0;
/* test if cursor is already on screen */
	if (cursor_state==cursor_state_visible) {
		do {
			if ((tsub==sub) && (tid1==id1) && (tid2==id2)) {
				cursor_sub=sub;
				cursor_id1=id1;
				cursor_id2=id2;
				if (tag_group!=-1) cursor_tag_group=tag_group;
				adjust_cursor_group();
				if (tag_idx!=-1) cursor_tag_idx=tag_idx;
				adjust_cursor_idx();
				cursor_ypos=ty;
				update_misc_info();
				update_visual_info();
				check_cursor_visibility();
				cursorline_dirty();
				dirtyview();
				return 1;
			}
		} while ((next_line(&tsub, &tid1, &tid2, 1)) && (ty++<size.h-1));
	}
/**/
	char line[1024];	/* FIXME: possible buffer overflow ! */
	char *e;
	sub->getline(line, id1, id2);
	e=tag_get_selectable_tag(line, 0, 0);
	if (!e) return 0;
	cursor_sub=sub;
	cursor_id1=id1;
	cursor_id2=id2;
	if (tag_group!=-1) cursor_tag_group=tag_group;
	adjust_cursor_group();
	if (tag_idx!=-1) cursor_tag_idx=tag_idx;
	adjust_cursor_idx();
	cursor_ypos=0;
//	cursor_ypos=center_view(sub, id1, id2);
	top_sub=sub;
	top_id1=id1;
	top_id2=id2;
	update_misc_info();
	update_visual_info();
	check_cursor_visibility();
	cursorline_dirty();
	dirtyview();
	return 1;
}

void ht_uformat_viewer::scroll_up(int n)
{
	cursor_ypos+=prev_line(&top_sub, &top_id1, &top_id2, n);
	cursorline_dirty();
	check_cursor_visibility();
}

void ht_uformat_viewer::scroll_down(int n)
{
	cursor_ypos-=next_line(&top_sub, &top_id1, &top_id2, n);
	cursorline_dirty();
	check_cursor_visibility();
}

bool ht_uformat_viewer::string_to_address(char *string, fmt_vaddress *addr)
{
	scalar_t r;
	if (eval(&r, string, NULL, NULL, NULL)) {
		int_t i;
		scalar_context_int(&r, &i);
		scalar_destroy(&r);
		*addr=i.value;
		return true;
	}
	char *s;
	int p;
	get_eval_error(&s, &p);
	sprintf(globalerror, "%s at pos %d", s, p);
	return false;
}

bool ht_uformat_viewer::string_to_offset(char *string, FILEOFS *ofs)
{
	scalar_t r;
	if (eval(&r, string, NULL, NULL, NULL)) {
		int_t i;
		scalar_context_int(&r, &i);
		scalar_destroy(&r);
		*ofs=i.value;
		return true;
	}
	char *s;
	int p;
	get_eval_error(&s, &p);
	sprintf(globalerror, "%s at pos %d", s, p);
	return false;
}

void ht_uformat_viewer::update_micropos()
{
	cursorline_get();
	char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	if (e) {
		int s=tag_get_microsize(e);
		if (cursor_tag_micropos>=s) cursor_tag_micropos=s-1;
	}
}

void ht_uformat_viewer::update_misc_info()
{
	cursorline_get();
	char *e=tag_get_selectable_tag(cursor_line, cursor_tag_idx, cursor_tag_group);
	if (e) {
		cursor_tag_class=tag_get_class(e);
		switch (cursor_tag_class) {
			case tag_class_edit:
				cursor_tag_offset=tag_get_offset(e);
				break;
			case tag_class_sel:
				tag_get_id(e, &cursor_tag_id.low, &cursor_tag_id.high);
				break;
		}
	}
}

void ht_uformat_viewer::update_visual_info()
{
	cursorline_get();
	char *s, *t=cursor_line;
	int v=0, vl=0;
	int i=0, g=0;
	while ((s=tag_findnext(t))) {
		int cl=tag_get_class(s);
		if (s[1]==HT_TAG_GROUP) {
			i=0;
			g++;
		}
		v+=s-t;
		vl=tag_get_vlen(s);
		if ((i==cursor_tag_idx) && (g==cursor_tag_group) && (cl!=tag_class_no)) break;
		v+=vl;
		t=s+tag_get_len(s);
		if (cl!=tag_class_no) i++;
	}

	if (cursor_tag_micropos > vl-1) cursor_tag_micropos = vl ? vl-1 : 0;
	cursor_visual_xpos=v;
	cursor_visual_length=vl;
}

void ht_uformat_viewer::update_ypos()
{
	ht_sub *sub=top_sub;
	ID id1=top_id1, id2=top_id2;
	int y=0;
	while ((next_line(&sub, &id1, &id2, 1)) && (y<size.h)) {
		if ((cursor_sub==sub) && (cursor_id1==id1) && (cursor_id2==id2)) {
			cursor_ypos=y;
			break;
		}
		y++;
	}
}

struct ht_uformat_view_vs {
	int edit;
	ht_sub *first_sub, *last_sub;
/* top line position */
	ht_sub *top_sub;
	ID top_id1, top_id2;
/* cursor line and tag position */
	int cursor_state;
	int cursor_ypos;
	ht_sub *cursor_sub;
	ID cursor_id1, cursor_id2;
	int cursor_tag_idx;
	int cursor_tag_group;
/* selection*/
	FILEOFS sel_start;
	FILEOFS sel_end;
};

void ht_uformat_viewer::view_state_load(void *data)
{
	ht_uformat_view_vs *vs=(ht_uformat_view_vs *)data;
	first_sub=vs->first_sub;
	last_sub=vs->last_sub;
/* top line position */
	top_sub=vs->top_sub;
	top_id1=vs->top_id1;
	top_id2=vs->top_id2;
/* cursor line and tag position */
	cursor_state=vs->cursor_state;
	cursor_ypos=vs->cursor_ypos;
	cursor_sub=vs->cursor_sub;
	cursor_id1=vs->cursor_id1;
	cursor_id2=vs->cursor_id2;
	cursor_tag_idx=vs->cursor_tag_idx;
	cursor_tag_group=vs->cursor_tag_group;
/* selection*/
	sel_start=vs->sel_start;
	sel_end=vs->sel_end;
/**/
	cursorline_dirty();
	update_misc_info();
	update_visual_info();
}

void *ht_uformat_viewer::view_state_create()
{
	ht_uformat_view_vs *vs=new ht_uformat_view_vs;
	vs->first_sub=first_sub;
	vs->last_sub=last_sub;
/* top line position */
	vs->top_sub=top_sub;
	vs->top_id1=top_id1;
	vs->top_id2=top_id2;
/* cursor line and tag position */
	vs->cursor_state=cursor_state;
	vs->cursor_ypos=cursor_ypos;
	vs->cursor_sub=cursor_sub;
	vs->cursor_id1=cursor_id1;
	vs->cursor_id2=cursor_id2;
	vs->cursor_tag_idx=cursor_tag_idx;
	vs->cursor_tag_group=cursor_tag_group;
/* selection*/
	vs->sel_start=sel_start;
	vs->sel_end=sel_end;
/**/
	return vs;
}

/*
 *	CLASS ht_sub
 */

void ht_sub::init(ht_streamfile *f)
{
	object::init();
	uformat_viewer=NULL;
	prev=NULL;
	next=NULL;
	file=f;
}

void ht_sub::done()
{
	object::done();
}

bool ht_sub::closest_line_id(ID *id1, ID *id2)
{
	first_line_id(id1, id2);
	return true;
}

bool ht_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	return false;
}

bool ht_sub::convert_ofs_to_id(FILEOFS offset, ID *id1, ID *id2)
{
	return false;
}

bool ht_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	return false;
}

bool ht_sub::convert_id_to_ofs(ID id1, ID id2, FILEOFS *offset)
{
	return false;
}

bool ht_sub::getline(char *line, ID id1, ID id2)
{
	return false;
}

void ht_sub::handlemsg(htmsg *msg)
{
}

void ht_sub::first_line_id(ID *id1, ID *id2)
{
}

void ht_sub::last_line_id(ID *id1, ID *id2)
{
}

int ht_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	return 0;
}

int ht_sub::next_line_id(ID *id1, ID *id2, int n)
{
	return 0;
}

bool ht_sub::ref(ID id1, ID id2)
{
	return false;
}

ht_search_result *ht_sub::search(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	return NULL;
}

/*
 *	CLASS ht_linear_sub
 */

void ht_linear_sub::init(ht_streamfile *f, FILEOFS ofs, int size)
{
	ht_sub::init(f);
	fofs=ofs;
	fsize=size;
}

void ht_linear_sub::done()
{
	ht_sub::done();
}

void ht_linear_sub::handlemsg(htmsg *msg)
{
	if (msg->msg==msg_filesize_changed) {
		UINT s=file->get_size();
		if (fofs>s) {
			fsize=0;
		} else if (fofs+fsize>s) {
			fsize=s-fofs;
		}
		return;
	}
}

int ht_linear_func_readbyte(scalar_t *result, int_t *offset)
{
	struct context_t {
		ht_linear_sub *sub;
		ht_format_viewer *fv;
		int i, o;
	};
	ht_format_viewer *f=((context_t*)eval_get_context())->fv;
	byte b;
	if (f->pread(offset->value, &b, 1)!=1) {
		set_eval_error("i/o error (requested %d, read %d from ofs %08x)", 1, 0, offset->value);
		return 0;
	}
	scalar_create_int_c(result, b);
	return 1;
}

int ht_linear_func_readstring(scalar_t *result, int_t *offset, int_t *len)
{
	struct context_t {
		ht_linear_sub *sub;
		ht_format_viewer *fv;
		int i, o;
	};
	ht_format_viewer *f=((context_t*)eval_get_context())->fv;

	UINT l=len->value;
	void *buf=malloc(l);	/* FIXME: may be too slow... */

	if (buf) {
		str_t s;
		UINT c=f->pread(offset->value, buf, l);
		if (c!=l) {
			free(buf);
			set_eval_error("i/o error (requested %d, read %d from ofs %08x)", l, c, offset->value);
			return 0;
		}
		s.value=(char*)buf;
		s.len=l;
		scalar_create_str(result, &s);
		free(buf);
		return 1;
	}
	set_eval_error("out of memory");
	return 0;
}

int ht_linear_func_entropy(scalar_t *result, str_t *buf)
{
	scalar_create_int_c(result, calc_entropy2((byte *)buf->value, buf->len));
	return 1;
}

int ht_linear_func_entropy2(scalar_t *result, str_t *buf)
{
	scalar_create_float_c(result, calc_entropy((byte *)buf->value, buf->len));
	return 1;
}

struct search_expr_eval_context_t {
	ht_sub *sub;
	ht_format_viewer *fv;
	int i, o;
};

int ht_linear_sub_func_handler(scalar_t *result, char *name, scalarlist_t *params)
{
	evalfunc_t myfuncs[] = {
		{"entropy", (void*)&ht_linear_func_entropy, {SCALAR_STR}},
		{"entropy2", (void*)&ht_linear_func_entropy2, {SCALAR_STR}},
		{"readbyte", (void*)&ht_linear_func_readbyte, {SCALAR_INT}},
		{"readstring", (void*)&ht_linear_func_readstring, {SCALAR_INT, SCALAR_INT}},
		{NULL}
	};
	return std_eval_func_handler(result, name, params, myfuncs);
}

int ht_linear_sub_symbol_handler(scalar_t *result, char *name)
{
	search_expr_eval_context_t *context =
		(search_expr_eval_context_t*)eval_get_context();
	if (strcmp(name, "i")==0) {
		scalar_create_int_c(result, context->i);
		return 1;
	} else if (strcmp(name, "o")==0) {
		scalar_create_int_c(result, context->o);
		return 1;
	} else return 0;
}

class ht_expr_search_pcontext: public ht_data {
public:
/* in */
	ht_search_request *request;
	ht_sub *sub;
	ht_format_viewer *fv;
	FILEOFS start;
	FILEOFS end;
	int i;
	FILEOFS o;
/* out */
	ht_search_result **result;
};

bool process_search_expr(ht_data *ctx, ht_text *progress_indicator)
{
#define PROCESS_EXPR_SEARCH_BYTES_PER_CALL	256
	ht_expr_search_pcontext *c=(ht_expr_search_pcontext*)ctx;
	ht_expr_search_request *s=(ht_expr_search_request*)c->request;

	search_expr_eval_context_t context;
	context.sub = c->sub;
	context.fv = c->fv;
	int w=PROCESS_EXPR_SEARCH_BYTES_PER_CALL;
	while (c->o < c->end) {
		scalar_t r;
		context.i = c->i;
		context.o = c->o;
		if (eval(&r, s->expr, ht_linear_sub_func_handler, ht_linear_sub_symbol_handler, &context)) {
			int_t i;
			scalar_context_int(&r, &i);
			if (i.value) {
				ht_physical_search_result *r=new ht_physical_search_result();
				r->offset = c->o;
				r->size = 1;
				*c->result = r;
				return false;
			}
		} else {
			char *str;
			int pos;
			get_eval_error(&str, &pos);
			throw new ht_io_exception("eval error at pos %d: %s", pos, str);
		}
		c->i++;
		c->o++;
		
		if (!--w) {
			char text[64];
			if (c->end > c->start) {
				sprintf(text, "%d %% done", (c->o - c->start) * 100 / (c->end - c->start));
			} else {
				strcpy(text, "? % done");
			}
			progress_indicator->settext(text);
	
			return true;
		}
	}
	return false;
}

ht_search_result *linear_expr_search(ht_search_request *search, FILEOFS start, FILEOFS end, ht_sub *sub, ht_uformat_viewer *ufv, FILEOFS fofs, dword fsize)
{
	if (start<fofs) start=fofs;
	if (end>fofs+fsize) end=fofs+fsize;
	if (fsize) {
		ht_search_result *r=NULL;
		ht_expr_search_pcontext c;
		c.request=search;
		c.sub=sub;
		c.fv=ufv;
		c.start=start;
		c.end=end;
		c.result=&r;
		c.i=0;
		c.o=start;
		if (execute_process(process_search_expr, &c)) return r;
	}
	return NULL;
}

ht_search_result *linear_bin_search(ht_search_request *search, FILEOFS start, FILEOFS end, ht_streamfile *file, FILEOFS fofs, dword fsize)
{
	ht_fxbin_search_request *s=(ht_fxbin_search_request*)search;
		
	int fl=(search->flags & SFBIN_CASEINSENSITIVE) ? SFBIN_CASEINSENSITIVE : 0;
	if (start<fofs) start=fofs;
	if (end>fofs+fsize) end=fofs+fsize;
	if ((fsize) && (start<end)) {
		/* create result */
		bool search_success = false;
		FILEOFS search_ofs;
		ht_data *ctx = create_search_bin_context(file, start, end-start, s->data, s->data_size, fl, &search_ofs, &search_success);
		if (execute_process(search_bin_process, ctx)) {
			delete ctx;
			if (search_success) {
				ht_physical_search_result *r=new ht_physical_search_result();
				r->offset = search_ofs;
				r->size = s->data_size;
				return r;
			}
		} else delete ctx;
	}
	return NULL;
}

ht_search_result *ht_linear_sub::search(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	ht_search_result *r = NULL;
	if ((search->search_class==SC_PHYSICAL) && (search->type==ST_EXPR)) {
		r = linear_expr_search(search, start, end, this, uformat_viewer, fofs, fsize);
	} else if ((search->search_class==SC_PHYSICAL) && (search->type==ST_FXBIN)) {
		r = linear_bin_search(search, start, end, file, fofs, fsize);
	}
	return r;
}

/*
 *	CLASS ht_hex_sub
 */

void ht_hex_sub::init(ht_streamfile *f, FILEOFS ofs, dword size, UINT u, dword vinc)
{
	ht_linear_sub::init(f, ofs, size);
	vaddrinc=vinc;
	balign=ofs & 0xf;
	uid=u;
}

void ht_hex_sub::done()
{
	ht_linear_sub::done();
}

bool ht_hex_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	return convert_ofs_to_id(addr, id1, id2);
}

bool ht_hex_sub::convert_ofs_to_id(FILEOFS offset, ID *id1, ID *id2)
{
	if ((offset>=fofs) && (offset<fofs+fsize)) {
		*id1=offset&~15+balign;
		*id2=uid;
		return 1;
	}
	return 0;
}

bool ht_hex_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	*addr=id1;
	return 1;
}

bool ht_hex_sub::convert_id_to_ofs(ID id1, ID id2, FILEOFS *ofs)
{
	*ofs=id1;
	return 1;
}

bool ht_hex_sub::getline(char *line, ID id1, ID id2)
{
	if (id2!=uid) return 0;
	ID ofs=id1;
	dword c=MIN(16, (int)(fofs+fsize-ofs));
	if (c<=0) return 0;
	c = MIN(16, c+ofs%16);
	char *l=line;
	l=mkhexd(l, ofs+vaddrinc);
	*l++=' ';
	if (ofs % 16) ofs -= ofs % 16;
	for (dword i=0; i<16; i++) {
		if (i<c && ofs+i>=fofs) {
			l=tag_make_edit_byte(l, ofs+i);
		} else {
			*l++=' ';
			*l++=' ';
		}
		if (i+1<c && ofs+i>=fofs) {
			if (i==7) {
				l=tag_make_edit_selvis(l, ofs+i, '-');
			} else {
				l=tag_make_edit_selvis(l, ofs+i, ' ');
			}
		} else *l++=' ';
	}
	l=tag_make_group(l);
	*l++='|';
	for (dword i=0; i<16; i++) {
		if (i<c && ofs+i>=fofs) {
			l=tag_make_edit_char(l, ofs+i);
		} else {
			*l++=' ';
		}
	}
	*l++='|';
	*l=0;
	return 1;
}

void ht_hex_sub::handlemsg(htmsg *msg)
{
	ht_linear_sub::handlemsg(msg);
}

void ht_hex_sub::first_line_id(ID *id1, ID *id2)
{
	*id1=fofs;
	*id2=uid;
}

void ht_hex_sub::last_line_id(ID *id1, ID *id2)
{
	if (fsize) {
		*id1=((fsize) & ~0xf)+fofs;
	} else {
		*id1=fofs;
	}
	*id2=uid;
}

int ht_hex_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	if (*id2!=uid) return 0;
	int c=0;
	while (n--) {
		if (!*id1 || *id1 == fofs) break;
		if (*id1-16>*id1) {
			*id1 = 0;
		} else {
			if (*id1-16<fofs) {
				*id1 = fofs;
			} else {
				*id1 -= 16;
			}
		}
		c++;
	}
	return c;
}

int ht_hex_sub::next_line_id(ID *id1, ID *id2, int n)
{
	if (*id2!=uid) return 0;
	int c=0;
	while (n--) {
		if (*id1 % 16) {
			if (*id1+(16-*id1%16)>=fofs+fsize) break;
			*id1 += 16-*id1%16;
		} else {
			if (*id1+16>=fofs+fsize) break;
			*id1 += 16;
		}
		c++;
	}
	return c;
}

/*
 *	CLASS ht_mask
 */

void ht_mask_sub::init(ht_streamfile *f, UINT u, fmt_vaddress ba)
{
	ht_sub::init(f);
	masks = new ht_string_list();
	masks->init();
	uid = u;
	baseaddr = ba;
}

void ht_mask_sub::done()
{
	masks->destroy();
	delete masks;
	ht_sub::done();
}

bool ht_mask_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	if ((addr<baseaddr) || (addr>baseaddr+masks->count()-1)) return false;
	*id2 = uid;
	*id1 = addr-baseaddr;
	return true;
}

bool ht_mask_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	if (id2 != uid) return false;
	if (id1 > masks->count()-1) return false;
	*addr = baseaddr+id1;
	return true;
}

void ht_mask_sub::first_line_id(ID *id1, ID *id2)
{
	*id1=0;
	*id2=uid;
}

bool ht_mask_sub::getline(char *line, ID id1, ID id2)
{
	if (id2!=uid) return false;
	char *s=masks->get_string(id1);
	if (s) {
		tag_strcpy(line, s);
		return true;
	}
	return false;
}

void ht_mask_sub::last_line_id(ID *id1, ID *id2)
{
	*id1=masks->count()-1;
	*id2=uid;
}

int ht_mask_sub::next_line_id(ID *id1, ID *id2, int n)
{
	int r=n;
	if (*id2!=uid) return 0;
	int c=masks->count();
	ID i1=*id1;
	i1+=n;
	if ((int)i1>c-1) {
		r-=i1-c+1;
		i1=c-1;
	}
	if (r) *id1=i1;
	return r;
}

int ht_mask_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	int r;
	if (*id2!=uid) return 0;
	ID i1=*id1;
	if (i1<(dword)n) {
		r=i1;
		i1=0;
	} else {
		r=n;
		i1-=n;
	}
	if (r) *id1=i1;
	return r;
}

void ht_mask_sub::add_mask(char *tagstr)
{
	masks->insert(new ht_data_tagstring(tagstr));
}

void ht_mask_sub::add_mask_table(char **tagstr)
{
	while (*tagstr) add_mask(*(tagstr++));
}

void ht_mask_sub::add_staticmask(char *statictag_str, FILEOFS reloc, bool std_bigendian)
{
	char tag_str[1024];	/* FIXME: possible buffer overflow */
	statictag_to_tag(statictag_str, tag_str, reloc, std_bigendian);
	masks->insert(new ht_data_tagstring(tag_str));
}

void ht_mask_sub::add_staticmask_table(char **statictag_table, FILEOFS reloc, bool std_bigendian)
{
	while (*statictag_table) add_staticmask(*(statictag_table++), reloc, std_bigendian);
}

#define ht_MASK_STD_INDENT	50

void ht_mask_sub::add_staticmask_ptable(ht_mask_ptable *statictag_ptable, FILEOFS reloc, bool std_bigendian)
{
	char s[1024]; /* FIXME: possible buffer overflow */
	while (statictag_ptable->desc || statictag_ptable->fields) {
		s[0]=0;
		if (statictag_ptable->desc) strcpy(s, statictag_ptable->desc);
		int n=strlen(s);
		while (n<ht_MASK_STD_INDENT) {
			s[n]=' ';
			n++;
		}
		s[n]=0;
		if (statictag_ptable->fields) strcat(s, statictag_ptable->fields);

		add_staticmask(s, reloc, std_bigendian);
		
		statictag_ptable++;
	}
}

/*
 *	CLASS ht_layer_sub
 */

void ht_layer_sub::init(ht_streamfile *file, ht_sub *s, bool own_s=1)
{
	ht_sub::init(file);
	sub=s;
	own_sub=own_s;
}

void ht_layer_sub::done()
{
	if (own_sub) {
		sub->done();
		delete sub;
	}
	ht_sub::done();
}

bool ht_layer_sub::closest_line_id(ID *id1, ID *id2)
{
	return sub->closest_line_id(id1, id2);
}

bool ht_layer_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	return sub->convert_addr_to_id(addr, id1, id2);
}

bool ht_layer_sub::convert_ofs_to_id(FILEOFS offset, ID *id1, ID *id2)
{
	return sub->convert_ofs_to_id(offset, id1, id2);
}

bool ht_layer_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	return sub->convert_id_to_addr(id1, id2, addr);
}

bool ht_layer_sub::convert_id_to_ofs(ID id1, ID id2, FILEOFS *offset)
{
	return sub->convert_id_to_ofs(id1, id2, offset);
}

void ht_layer_sub::first_line_id(ID *id1, ID *id2)
{
	return sub->first_line_id(id1, id2);
}

bool ht_layer_sub::getline(char *line, ID id1, ID id2)
{
	return sub->getline(line, id1, id2);
}

void ht_layer_sub::handlemsg(htmsg *msg)
{
	sub->handlemsg(msg);
}

void ht_layer_sub::last_line_id(ID *id1, ID *id2)
{
	return sub->last_line_id(id1, id2);
}

int ht_layer_sub::next_line_id(ID *id1, ID *id2, int n)
{
	return sub->next_line_id(id1, id2, n);
}

int ht_layer_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	return sub->prev_line_id(id1, id2, n);
}

bool ht_layer_sub::ref(ID id1, ID id2)
{
	return sub->ref(id1, id2);
}

ht_search_result *ht_layer_sub::search(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	return sub->search(search, start, end);
}

/*
 *	CLASS ht_collapsable_sub
 */

fmt_vaddress ht_collapsable_sub_globalfaddr=0xffffffff;

void ht_collapsable_sub::init(ht_streamfile *file, ht_sub *sub, bool own_sub, char *_nodestring, bool _collapsed)
{
	ht_layer_sub::init(file, sub, own_sub);
	nodestring=ht_strdup(_nodestring);
	collapsed=_collapsed;
	ht_layer_sub::first_line_id(&fid1, &fid2);
//	myfid1=0x12345678;	// it's kinda magic
//	myfid2=0x35043859;
	myfid1=0;
	myfid2=ht_collapsable_sub_globalfaddr--;
	myfaddr=ht_collapsable_sub_globalfaddr--;
}

void ht_collapsable_sub::done()
{
	if (nodestring) delete nodestring;
	ht_layer_sub::done();
}

bool ht_collapsable_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	if (addr==myfaddr) {
		first_line_id(id1, id2);
		return 1;
	} else if (!collapsed) {
		return ht_layer_sub::convert_addr_to_id(addr, id1, id2);
	}
	return 0;
}

bool ht_collapsable_sub::convert_ofs_to_id(FILEOFS offset, ID *id1, ID *id2)
{
	if (offset==myfaddr) {
		first_line_id(id1, id2);
		return 1;
	} else if (!collapsed) {
		return ht_layer_sub::convert_ofs_to_id(offset, id1, id2);
	}
	return 0;
}

bool ht_collapsable_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	if ((id1==myfid1) && (id2==myfid2)) {
		*addr=myfaddr;
		return 1;
	}
	return ht_layer_sub::convert_id_to_addr(id1, id2, addr);
}

bool ht_collapsable_sub::convert_id_to_ofs(ID id1, ID id2, FILEOFS *offset)
{
	if ((id1==myfid1) && (id2==myfid2)) {
		*offset=myfaddr;
		return 1;
	}
	return ht_layer_sub::convert_id_to_ofs(id1, id2, offset);
}

void ht_collapsable_sub::first_line_id(ID *id1, ID *id2)
{
	*id1=myfid1;
	*id2=myfid2;
}

bool ht_collapsable_sub::getline(char *line, ID id1, ID id2)
{
	if ((id1==myfid1) && (id2==myfid2)) {
		line+=sprintf(line, "[%c] ", collapsed ? '+' : '-');
		line=tag_make_ref(line, myfid1, myfid2, nodestring);
		*line=0;
		return 1;
	} else if (collapsed) return 0;
	if (ht_layer_sub::getline(line, id1, id2)) {
		memmove(line+2, line, tag_strlen(line)+1);
		line[0]=' ';
		line[1]=' ';
		return 1;
	}
	return 0;
}

void ht_collapsable_sub::last_line_id(ID *id1, ID *id2)
{
	if (collapsed) return first_line_id(id1, id2); else
		return ht_layer_sub::last_line_id(id1, id2);
}

int ht_collapsable_sub::next_line_id(ID *id1, ID *id2, int n)
{
	if (collapsed) return 0;
	int r=0;
	ID t1, t2;
	if ((*id1==myfid1) && (*id2==myfid2)) {
		ht_layer_sub::first_line_id(&t1, &t2);
		n--;
		r++;
	} else {
		t1=*id1;
		t2=*id2;
	}
	if (n) r+=ht_layer_sub::next_line_id(&t1, &t2, n);
	if (r) {
		*id1=t1;
		*id2=t2;
	}
	return r;
}

int ht_collapsable_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	if (collapsed) return 0;
	if ((*id1==myfid1) && (*id2==myfid2)) return 0;
	int r=ht_layer_sub::prev_line_id(id1, id2, n);
	if (((*id1==fid1) && (*id2==fid2)) && (r<n)) {
		*id1=myfid1;
		*id2=myfid2;
		r++;
	}
	return r;
}

bool ht_collapsable_sub::ref(ID id1, ID id2)
{
	if ((id1==myfid1) && (id2==myfid2)) {
		collapsed=!collapsed;
		return 1;
	}
	if (!collapsed) return ht_layer_sub::ref(id1, id2);
	return 0;
}

ht_search_result *ht_collapsable_sub::search(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	if (collapsed) return 0;
	return ht_layer_sub::search(search, start, end);
}

/*
 *	CLASS ht_group_sub
 */

void ht_group_sub::init(ht_streamfile *file)
{
	ht_sub::init(file);
	subs=new ht_clist();
	subs->init();
}

void ht_group_sub::done()
{
	subs->done();
	delete subs;
	ht_sub::done();
}

bool ht_group_sub::convert_addr_to_id(fmt_vaddress addr, ID *id1, ID *id2)
{
	return false;
}

bool ht_group_sub::convert_ofs_to_id(FILEOFS offset, ID *id1, ID *id2)
{
	return false;
}

bool ht_group_sub::convert_id_to_addr(ID id1, ID id2, fmt_vaddress *addr)
{
	return false;
}

bool ht_group_sub::convert_id_to_ofs(ID id1, ID id2, FILEOFS *offset)
{
	return false;
}

void ht_group_sub::first_line_id(ID *id1, ID *id2)
{
	ht_sub *s=(ht_sub*)subs->get(0);
	if (s) s->first_line_id(id1, id2);
}

bool ht_group_sub::getline(char *line, ID id1, ID id2)
{
	ht_sub *s;
	UINT c=subs->count();
	for (UINT i=0; i<c; i++) {
		s=(ht_sub*)subs->get(i);
		if (s->getline(line, id1, id2)) return true;
	}
	return false;
}

void ht_group_sub::handlemsg(htmsg *msg)
{
	ht_sub::handlemsg(msg);
}

void ht_group_sub::last_line_id(ID *id1, ID *id2)
{
	ht_sub *s=(ht_sub*)subs->get(subs->count()-1);
	if (s) s->last_line_id(id1, id2);
}

int ht_group_sub::next_line_id(ID *id1, ID *id2, int n)
{
	ht_sub *s;
	UINT c=subs->count();
	int on=n;
	for (UINT i=0; i<c; i++) {
		s=(ht_sub*)subs->get(i);
		ID t1, t2;
		s->last_line_id(&t1, &t2);
		if ((t1==*id1) && (t2==*id2)) {
			s=(ht_sub*)subs->get(i+1);
			if (s) {
				s->first_line_id(id1, id2);
				n--;
			}
		} else {
			n-=s->next_line_id(id1, id2, n);
		}
		if (!n) break;
	}
	return on-n;
}

int ht_group_sub::prev_line_id(ID *id1, ID *id2, int n)
{
	ht_sub *s;
	UINT c=subs->count();
	int on=n;
	for (UINT i=0; i<c; i++) {
		s=(ht_sub*)subs->get(i);
		ID t1, t2;
		s->first_line_id(&t1, &t2);
		if ((t1==*id1) && (t2==*id2)) {
			s=(ht_sub*)subs->get(i-1);
			if (s) {
				s->last_line_id(id1, id2);
				n--;
			}
		} else {
			n-=s->prev_line_id(id1, id2, n);
		}
		if (!n) break;
	}
	return on-n;
}

bool ht_group_sub::ref(ID id1, ID id2)
{
	ht_sub *s;
	UINT c=subs->count();
	for (UINT i=0; i<c; i++) {
		s=(ht_sub*)subs->get(i);
		if (s->ref(id1, id2)) return true;
	}
	return false;
}

ht_search_result *ht_group_sub::search(ht_search_request *search, FILEOFS start, FILEOFS end)
{
	return NULL;
}

void ht_group_sub::insertsub(ht_sub *sub)
{
	subs->insert(sub);
}

/*
 *	CLASS ht_data_tagstring
 */

ht_data_tagstring::ht_data_tagstring(char *tagstr)
: ht_data_string()
{
	value=tag_strdup(tagstr);
}

ht_data_tagstring::~ht_data_tagstring()
{
}


