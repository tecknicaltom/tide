/* 
 *	HT Editor
 *	common.h
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

#ifndef COMMON_H
#define COMMON_H

#include "global.h"
#include "htdebug.h"

/**
 *	Macro for creating object build functions
 */
#define BUILDER(reg, obj) Object *build_##obj(){return new obj();}

/**
 *	Registers builder function by object id.
 */
#define REGISTER(reg, obj) register_atom(reg, (void*)build_##obj);

/**
 *	Unregisters builder function by object id.
 */
#define UNREGISTER(reg, obj) unregister_atom(reg);

/* actually a str => bigendian-int */
/** used to define OBJECT_IDs */
#define MAGICD(magic) (unsigned long)(((unsigned char)magic[0]<<24) | ((unsigned char)magic[1]<<16) | ((unsigned char)magic[2]<<8) | (unsigned char)magic[3])

#define ATOM_OBJECT MAGICD("OBJ0")

class ht_object_stream;

/**
 *	This is THE base class.
 */
class Object {
public:
#ifdef HTDEBUG
	bool initialized;
	bool destroyed;
				Object();
	virtual			~Object();

		void		init();
	virtual	void		done();
#else

				Object() {};
	virtual			~Object() {};

		void		init() {};
	virtual	void		done() {};
#endif
	virtual	int		compareTo(const Object *obj) const;
	virtual	Object *	duplicate();
	virtual	bool		idle();
	virtual	bool		instanceOf(OBJECT_ID id);
		bool		instanceOf(Object *o) {return instanceOf(o->object_id());};
	virtual	int		load(ht_object_stream *s) {return 0;};
	virtual	OBJECT_ID	object_id() const;
	virtual	void		store(ht_object_stream *s) {};
	virtual	int		toString(char *s, int maxlen);
};

#endif
