/*
 *	HT Editor
 *	xex_analy.cc
 *
 *	Copyright (C) 2006 Sebastian Biallas (sb@biallas.net)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analy.h"
#include "analy_names.h"
#include "analy_register.h"
#include "analy_ppc.h"
#include "htctrl.h"
#include "htdebug.h"
#include "htiobox.h"
#include "htxex.h"
#include "strtools.h"
#include "xex_analy.h"
#include "xexstruct.h"
#include "snprintf.h"

void	XEXAnalyser::init(ht_xex_shared_data *XEX_shared, File *File)
{
	xex_shared = XEX_shared;
	file = File;

	validarea = new Area();
	validarea->init();

	Analyser::init();
}

/*
 *
 */
void	XEXAnalyser::load(ObjectStream &f)
{
	GET_OBJECT(f, validarea);
	Analyser::load(f);
}

/*
 *
 */
void	XEXAnalyser::done()
{
	validarea->done();
	delete validarea;
	Analyser::done();
}

/*
 *
 */
void XEXAnalyser::beginAnalysis()
{
	char	buffer[1024];
	
	Address *entry = createAddress32(xex_shared->entrypoint);
	pushAddress(entry, entry);

#if 0
	// exports

	int export_count=xex_shared->exports.funcs->count();
	int *entropy = random_permutation(export_count);
	for (int i=0; i<export_count; i++) {
		ht_pe_export_function *f=(ht_pe_export_function *)(*xex_shared->exports.funcs)[entropy[i]];
		Address *faddr;
		if (pe32) {
			faddr = createAddress32(f->address + xex_shared->pe32.header_nt.image_base);
		} else {
			faddr = createAddress64(f->address + xex_shared->pe64.header_nt.image_base);
		}
		if (validAddress(faddr, scvalid)) {
			char *label;
			if (f->byname) {
				ht_snprintf(buffer, sizeof buffer, "; exported function %s, ordinal %04x", f->name, f->ordinal);
			} else {
				ht_snprintf(buffer, sizeof buffer, "; unnamed exported function, ordinal %04x", f->ordinal);
			}
			label = export_func_name((f->byname) ? f->name : NULL, f->ordinal);
			addComment(faddr, 0, "");
			addComment(faddr, 0, ";********************************************************");
			addComment(faddr, 0, buffer);
			addComment(faddr, 0, ";********************************************************");
			pushAddress(faddr, faddr);
			assignSymbol(faddr, label, label_func);
			free(label);
		}
		delete faddr;
	}
	if (entropy) free(entropy);

	int import_count=xex_shared->imports.funcs->count();
	entropy = random_permutation(import_count);
	for (int i=0; i<import_count; i++) {
		ht_pe_import_function *f = (ht_pe_import_function *)(*xex_shared->imports.funcs)[entropy[i]];
		ht_pe_import_library *d = (ht_pe_import_library *)(*xex_shared->imports.libs)[f->libidx];
		char *label;
		label = import_func_name(d->name, (f->byname) ? f->name.name : NULL, f->ordinal);
		Address *faddr;
		if (pe32) {
			faddr = createAddress32(f->address + xex_shared->pe32.header_nt.image_base);
		} else {
			faddr = createAddress64(f->address + xex_shared->pe64.header_nt.image_base);
		}
		addComment(faddr, 0, "");
		if (!assignSymbol(faddr, label, label_func)) {
			// multiple import of a function (duplicate labelname)
			// -> mangle name a bit more
			addComment(faddr, 0, "; duplicate import");
			ht_snprintf(buffer, sizeof buffer, "%s_%x", label, f->address);
			assignSymbol(faddr, buffer, label_func);
		}
		if (pe32) {
			data->setIntAddressType(faddr, dst_idword, 4);
		} else {
			data->setIntAddressType(faddr, dst_iqword, 8);
		}
		free(label);
		delete faddr;
	}
	if (entropy) free(entropy);

	int dimport_count=xex_shared->dimports.funcs->count();
	entropy = random_permutation(dimport_count);
	for (int i=0; i<dimport_count; i++) {
		// FIXME: delay imports need work (push addr)
		ht_pe_import_function *f=(ht_pe_import_function *)(*xex_shared->dimports.funcs)[entropy[i]];
		ht_pe_import_library *d=(ht_pe_import_library *)(*xex_shared->dimports.libs)[f->libidx];
		if (f->byname) {
			ht_snprintf(buffer, sizeof buffer, "; delay import function loader for %s, ordinal %04x", f->name.name, f->ordinal);
		} else {
			ht_snprintf(buffer, sizeof buffer, "; delay import function loader for ordinal %04x", f->ordinal);
		}
		char *label;
		label = import_func_name(d->name, f->byname ? f->name.name : NULL, f->ordinal);
		Address *faddr;
		if (pe32) {
			faddr = createAddress32(f->address);
		} else {
			faddr = createAddress64(f->address);
		}
		addComment(faddr, 0, "");
		addComment(faddr, 0, ";********************************************************");
		addComment(faddr, 0, buffer);
		addComment(faddr, 0, ";********************************************************");
		assignSymbol(faddr, label, label_func);
		free(label);
		delete faddr;
	}
	if (entropy) free(entropy);
#endif

	addComment(entry, 0, "");
	addComment(entry, 0, ";****************************");
	addComment(entry, 0, ";  program entry point");
	addComment(entry, 0, ";****************************");
	assignSymbol(entry, "entrypoint", label_func);

	setLocationTreeOptimizeThreshold(1000);
	setSymbolTreeOptimizeThreshold(1000);
	delete entry;

	Analyser::beginAnalysis();
}

/*
 *
 */
ObjectID	XEXAnalyser::getObjectID() const
{
	return ATOM_XEX_ANALYSER;
}

/*
 *
 */
uint XEXAnalyser::bufPtr(Address *Addr, byte *buf, int size)
{
	FileOfs ofs = addressToFileofs(Addr);
/*	if (ofs == INVALID_FILE_OFS) {
		int as=0;
	}*/
	assert(ofs != INVALID_FILE_OFS);
	file->seek(ofs);
	return file->read(buf, size);
}

bool XEXAnalyser::convertAddressToRVA(Address *addr, RVA *r)
{
	ObjectID oid = addr->getObjectID();
	if (oid == ATOM_ADDRESS_FLAT_32) {
		*r = ((AddressFlat32*)addr)->addr - xex_shared->image_base;
		return true;
	}
	return false;
}

/*
 *
 */
Address *XEXAnalyser::createAddress32(uint32 addr)
{
	return new AddressFlat32(addr);
}


Address *XEXAnalyser::createAddress()
{
	return new AddressFlat32();
}

/*
 *
 */
FileOfs XEXAnalyser::addressToFileofs(Address *Addr)
{
	if (validAddress(Addr, scinitialized)) {
		FileOfs ofs;
		RVA r;
		if (!convertAddressToRVA(Addr, &r)) {
//		fprintf(stderr, "111111: %08x\n", r);
//		assert(0);
		return INVALID_FILE_OFS;
		}
		if (!xex_rva_to_ofs(xex_shared, r, ofs)) {
//		fprintf(stderr, "222222: %08x\n", r);
//		assert(0);
		return INVALID_FILE_OFS;
		}
		return ofs;
	} else {
		return INVALID_FILE_OFS;
	}
}

/*
 *
 */
const char *XEXAnalyser::getSegmentNameByAddress(Address *Addr)
{
	return "";
}

/*
 *
 */
String &XEXAnalyser::getName(String &s)
{
	return file->getDesc(s);
}

/*
 *
 */
const char *XEXAnalyser::getType()
{
	return "XEx/Analyser";
}

/*
 *
 */
void XEXAnalyser::initUnasm()
{
	analy_disasm = new AnalyPPCDisassembler();
	((AnalyPPCDisassembler*)analy_disasm)->init(this, ANALY_PPC_32);
}

/*
 *
 */
Address *XEXAnalyser::nextValid(Address *Addr)
{
	return (Address *)validarea->findNext(Addr);
}

/*
 *
 */
void XEXAnalyser::store(ObjectStream &st) const
{
	PUT_OBJECT(st, validarea);
	Analyser::store(st);
}


/*
 *
 */
Address *XEXAnalyser::fileofsToAddress(FileOfs fileofs)
{
	RVA r;
	if (xex_ofs_to_rva(xex_shared, fileofs, r)) {
		return createAddress32(r + xex_shared->image_base);
	} else {
		return new InvalidAddress();
	}
}

/*
 *
 */
bool XEXAnalyser::validAddress(Address *Addr, tsectype action)
{
//	e_section_headers *sections=&xex_shared->sections;
	int sec;
	RVA r;
	FileOfs ofs;
	if (!convertAddressToRVA(Addr, &r)) return false;
//	if (!pe_rva_to_section(sections, r, &sec)) return false;
	if (!xex_rva_to_ofs(xex_shared, r, ofs)) return false;
	return true;
}
