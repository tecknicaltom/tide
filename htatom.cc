/* 
 *	HT Editor
 *	htatom.cc
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

#include "htatom.h"
#include "htdata.h"

#include <string.h>

ht_stree *atoms;

void *find_atom(HT_ATOM atom)
{
	if (atom) {    		// atom 0 is special !
		ht_data_uint a(atom);
		ht_data_ptr *d=(ht_data_ptr*)atoms->get(&a);
		if (d) return (void*)d->value;
	}
	return NULL;
}

HT_ATOM find_atom_rev(const void *data)
{
	if (data) {
		ht_data_uint *key=NULL;
		ht_data_ptr *value;
		while ((key=(ht_data_uint*)atoms->enum_next((Object**)&value, key))) {
			if (value->value==data) return key->value;
		}
	}
	return 0;
}

bool register_atom(HT_ATOM atom, const void *data)
{
	if (!find_atom(atom)) {
		atoms->insert(new ht_data_uint(atom), new ht_data_ptr(data));
		return true;
	}
	return false;
}

bool unregister_atom(HT_ATOM atom)
{
	ht_data_uint a(atom);
	atoms->del(&a);
	return true;
}

/*
 *	INIT
 */
 
bool init_atom()
{
	atoms=new ht_stree();
	atoms->init(compare_keys_uint);
	return true;
} 
 
/*
 *	DONE
 */
 
void done_atom()
{
	atoms->destroy();
	delete atoms;
}

