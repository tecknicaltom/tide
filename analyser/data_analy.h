/* 
 *	HT Editor
 *	data_analy.h
 *
 *	Copyright (C) 1999-2002 Sebastian Biallas (sb@web-productions.de)
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

#ifndef DATA_ANALY_H
#define DATA_ANALY_H

#include "global.h"
#include "analy.h"


typedef enum {op_read, op_write, op_offset} OP;

/*
 * general type of an address
 */
typedef enum {
	dt_unknown = 0,
	dt_code,
	dt_unknown_data,
	dt_int,
	dt_float,
	dt_array
} taddr_typetype;

typedef enum {
	dst_cunknown = 0,
	dst_location,
	dst_function
} taddr_code_subtype;

typedef enum {
	dst_iunknown = 0,
	dst_ibyte,
	dst_iword,
	dst_idword,
	dst_ipword,
	dst_iqword
} taddr_int_subtype;

typedef enum {
	dst_funknown = 0,
	dst_fdouble,
	dst_fextended
} taddr_float_subtype;

typedef enum {
	dst_aunknown = 0,
	dst_abyte,
	dst_aword,
	dst_adword,
	dst_apword,
	dst_aqword,
	dst_string,
	dst_unistring
} taddr_array_subtype;

struct taddr_type {
	taddr_typetype type;
	union {
		byte				subtype;
		taddr_code_subtype	code_subtype;
		taddr_int_subtype	int_subtype;
		taddr_float_subtype	float_subtype;
		taddr_array_subtype	array_subtype;
	};
	union {
		int  length;
		// ...?
	};
};

void analyser_put_addrtype(ht_object_stream *f, const taddr_type *at);
int analyser_get_addrtype(ht_object_stream *f, taddr_type *at);

class Analyser;
class Address;
struct Location;

class DataAnalyser: public Object	{
public:
	Analyser		*analy;
			void		init(Analyser *Analy);
			int		load(ht_object_stream *f);
	virtual	void		done();
	virtual	OBJECT_ID	object_id();

			void		access(Address *Addr, OP op, int size);
			void		setAddressType(Address *Addr, taddr_typetype type, int subtype, int length);
			void		setAddressType(Location *Addr, taddr_typetype type, int subtype, int length);
			void		setCodeAddressType(Address *Addr, taddr_code_subtype subtype);
			void		setCodeAddressType(Location *Addr, taddr_code_subtype subtype);
			void		setIntAddressType(Address *Addr, taddr_int_subtype subtype, int length);
			void		setIntAddressType(Location *Addr, taddr_int_subtype subtype, int length);
			void		setFloatAddressType(Address *Addr, taddr_float_subtype subtype, int length);
			void		setFloatAddressType(Location *Addr, taddr_float_subtype subtype, int length);
			void		setArrayAddressType(Address *Addr, taddr_array_subtype subtype, int length);
			void		setArrayAddressType(Location *Addr, taddr_array_subtype subtype, int length);
	virtual	void		store(ht_object_stream *f);
};

void analyser_put_addrtype(ht_object_stream *f, const taddr_type *at);
int analyser_get_addrtype(ht_object_stream *f, taddr_type *at);

#endif
