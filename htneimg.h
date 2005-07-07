/* 
 *	HT Editor
 *	htneimg.h
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

#ifndef __HTNEIMG_H__
#define __HTNEIMG_H__

#include "htanaly.h"
#include "htne.h"
#include "formats.h"

extern format_viewer_if htneimage_if;

/*
 *	CLASS ht_ne_aviewer
 */

class ht_ne_aviewer: public ht_aviewer {
public:
	ht_ne_shared_data *ne_shared;
	File *file;
	
		   void init(bounds *b, char *desc, int caps, File *file, ht_format_group *format_group, Analyser *Analyser, ht_ne_shared_data *ne_shared);
/* overwritten */
	virtual char *func(uint i, bool execute);
	virtual void setAnalyser(Analyser *a);
};

#endif /* !__HTNEIMG_H__ */

