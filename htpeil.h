/* 
 *	HT Editor
 *	htpeil.h
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

#ifndef __HTPEIL_H__
#define __HTPEIL_H__

#include "common.h"
#include "formats.h"
#include "htdata.h"
#include "ilstruct.h"

extern format_viewer_if htpeil_if;

class ht_il_metadata_entry: public Object {
public:
	char *name;
	dword offset;
	dword size;
	ht_il_metadata_entry(char *name, dword offset, dword size);
	~ht_il_metadata_entry();
};

class ht_pe_il: public Object {
public:
	PE_IL_DIRECTORY dir;
	IL_METADATA_SECTION metadata;
	ht_clist *entries;
	dword string_pool_size;
	char *string_pool;
};

/*
 *	CLASS ht_pe_header_viewer
 */

class ht_pe_il_viewer: public ht_uformat_viewer {
public:
			void init(bounds *b, char *desc, int caps, ht_streamfile *file, ht_format_group *group);
		virtual void done();
};

int ILunpackDword(dword &result, const byte *buf, int len);
int ILunpackToken(dword &result, const byte *buf, int len);

#endif /* !__HTPEIL_H__ */
