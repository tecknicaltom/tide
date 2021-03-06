/*
 *	HT Editor
 *	xbe_analy.h
 *
 *	Copyright (C) 2003 Stefan Esser
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

#ifndef xbe_analy_h
#define xbe_analy_h

#include "analy.h"
#include "htxbe.h"


class XBEAnalyser: public Analyser {
public:
	ht_xbe_shared_data 	*xbe_shared;
	File			*file;
	Area			*validarea;

				XBEAnalyser() {};
				XBEAnalyser(BuildCtorArg&a): Analyser(a) {};
		void		init(ht_xbe_shared_data *XBE_shared, File *File);
		void		load(ObjectStream &f);
	virtual	void		done();
	virtual	ObjectID	getObjectID() const;

	virtual	void		beginAnalysis();
	virtual	uint		bufPtr(const Address *Addr, byte *buf, int size);
		bool		convertAddressToRVA(const Address *addr, RVA *r);
	virtual	Address		*createAddress();
		Address		*createAddress32(uint32 addr);
		Address		*createAddress64(uint64 high_addr);
	virtual Assembler 	*createAssembler();
	virtual	String &	getName(String &res);
	virtual const char	*getType();
	virtual	void 		initCodeAnalyser();
	virtual	void 		initUnasm();
	virtual	void 		log(const char *msg);
	virtual	Address		*nextValid(const Address *Addr);
	virtual	void		store(ObjectStream &f) const;
	virtual	int		queryConfig(int mode);
	virtual	bool 		validAddress(const Address *Addr, tsectype action);
	virtual	Address		*fileofsToAddress(FileOfs fileofs);
	virtual	FileOfs		addressToFileofs(const Address *Addr);
	virtual	const char	*getSegmentNameByAddress(const Address *Addr);
};

#endif
