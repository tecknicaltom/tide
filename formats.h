/* 
 *	HT Editor
 *	formats.h
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

#ifndef __FORMATS_H__
#define __FORMATS_H__

struct format_viewer_if;

#include "stream.h"
#include "htformat.h"

struct format_viewer_if {
	ht_view *(*init)(bounds *b, ht_streamfile *file, ht_format_group *group);
	void (*done)(ht_view *view);
};

extern format_viewer_if *format_viewer_ifs[];

#endif /* !__FORMATS_H__ */
