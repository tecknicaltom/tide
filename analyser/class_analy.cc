/*
 *	HT Editor
 *	class_analy.cc
 *
 *	Copyright (C) 1999-2002 Sebastian Biallas (sb@biallas.net)
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

#include "analy.h"
#include "analy_alpha.h"
#include "analy_names.h"
#include "analy_register.h"
#include "analy_java.h"
#include "class.h"
#include "class_analy.h"

#include "htctrl.h"
#include "htdebug.h"
#include "htiobox.h"
#include "strtools.h"
#include "snprintf.h"
#include "pestruct.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *
 */
void	ClassAnalyser::init(ht_class_shared_data *Class_shared, File *File)
{
	class_shared = Class_shared;
	file = File;

	Analyser::init();

	initialized->done();
	delete initialized;
	initialized = class_shared->initialized;
	/////////////

	setLocationTreeOptimizeThreshold(100);
	setSymbolTreeOptimizeThreshold(100);
}


/*
 *
 */
void	ClassAnalyser::load(ObjectStream &f)
{
	Analyser::load(f);
}

/*
 *
 */
void	ClassAnalyser::done()
{
	Analyser::done();
}

/*
 *
 */
void ClassAnalyser::beginAnalysis()
{
	char buffer[1024];
	char *b = buffer;
	
	*(b++) = ';';  *(b++) = ' ';
	b = java_demangle_flags(b, class_shared->flags);
	b += ht_snprintf(b, 1024, "%s %s", (class_shared->flags & jACC_INTERFACE)?"interface":"class", class_shared->classinfo.thisclass);
	if (class_shared->classinfo.superclass) {
		b += ht_snprintf(b, 1024, " extends %s", class_shared->classinfo.superclass);
	}
	if (class_shared->classinfo.interfaces) {
		b += ht_snprintf(b, 1024, " implements");
		int count = class_shared->classinfo.interfaces->count();
		for (int i=0; i<count; i++) {
			b += ht_snprintf(b, 1024, " %y%c", (*class_shared->classinfo.interfaces)[i], (i+1<count)?',':' ');
		}
	}
	b += ht_snprintf(b, 1024, " {");

	Address *a = createAddress32(0);
	addComment(a, 0, "");
	addComment(a, 0, ";********************************************************");
	addComment(a, 0, buffer);
	addComment(a, 0, ";********************************************************");
	delete a;
	if (class_shared->methods) {
		foreach (ClassMethod, cm, *class_shared->methods, {
			Address *a = createAddress32(cm->start);
			char buffer2[1024];
			java_demangle(buffer2, class_shared->classinfo.thisclass, cm->name, cm->type, cm->flags);
			ht_snprintf(buffer, 1024, "; %s", buffer2);
			addComment(a, 0, "");
			addComment(a, 0, ";----------------------------------------------");
			addComment(a, 0, buffer);
			addComment(a, 0, ";----------------------------------------------");
			addAddressSymbol(a, cm->name, label_func);
			pushAddress(a, a);
			delete a;
		});
	}
	setLocationTreeOptimizeThreshold(1000);
	setSymbolTreeOptimizeThreshold(1000);

	Analyser::beginAnalysis();
}

/*
 *
 */
ObjectID	ClassAnalyser::getObjectID() const
{
	return ATOM_CLASS_ANALYSER;
}

/*
 *
 */
uint ClassAnalyser::bufPtr(Address *Addr, byte *buf, int size)
{
	FileOfs ofs = addressToFileofs(Addr);
	assert(ofs != INVALID_FILE_OFS);
	file->seek(ofs);
	return file->read(buf, size);
}

/*
 *
 */
Address *ClassAnalyser::createAddress()
{
	return new AddressFlat32(0);
}

/*
 *
 */
Address *ClassAnalyser::createAddress32(ClassAddress addr)
{
	return new AddressFlat32((uint32)addr);
}

/*
 *
 */
Assembler *ClassAnalyser::createAssembler()
{
	return NULL;
}

/*
 *
 */
FileOfs ClassAnalyser::addressToFileofs(Address *Addr)
{
	if (validAddress(Addr, scinitialized)) {
		return ((AddressFlat32*)Addr)->addr;
	} else {
		return INVALID_FILE_OFS;
	}
}

/*
 *
 */
const char *ClassAnalyser::getSegmentNameByAddress(Address *Addr)
{
	static char sectionname[9];
	strcpy(sectionname, "test");
	return sectionname;
}

/*
 *
 */
String &ClassAnalyser::getName(String &res)
{
	return file->getDesc(res);
}

/*
 *
 */
const char *ClassAnalyser::getType()
{
	return "Java-Class/Analyser";
}

/*
 *
 */
void ClassAnalyser::initCodeAnalyser()
{
	Analyser::initCodeAnalyser();
}


int class_token_func(char *result, int maxlen, uint32 token, void *context)
{
	return token_translate(result, maxlen, token, (ht_class_shared_data *)context);
}

/*
 *
 */
void ClassAnalyser::initUnasm()
{
	DPRINTF("class_analy: ");
	analy_disasm = new AnalyJavaDisassembler();
	((AnalyJavaDisassembler*)analy_disasm)->init(this, class_token_func, class_shared);
}

/*
 *
 */
void ClassAnalyser::log(const char *msg)
{
	/*
	 *	log() creates to much traffic so dont log
	 *   perhaps we reactivate this later
	 *
	 */
/*	LOG(msg);*/
}

/*
 *
 */
Address *ClassAnalyser::nextValid(Address *Addr)
{
	return (Address *)class_shared->valid->findNext(Addr);
}

/*
 *
 */
void ClassAnalyser::store(ObjectStream &st) const
{
	Analyser::store(st);
}

/*
 *
 */
int	ClassAnalyser::queryConfig(int mode)
{
	switch (mode) {
		case Q_DO_ANALYSIS:
		case Q_ENGAGE_CODE_ANALYSER:
		case Q_ENGAGE_DATA_ANALYSER:
			return true;
		default:
			return 0;
	}
}

/*
 *
 */
Address *ClassAnalyser::fileofsToAddress(FileOfs fileaddr)
{
	Address *a = createAddress32(fileaddr);
	if (validAddress(a, scvalid)) {
		return a;
	} else {
		delete a;
		return NULL;
	}
}

/*
 *
 */
bool ClassAnalyser::validAddress(Address *Addr, tsectype action)
{
	if (!Addr->isValid() || !class_shared->valid->contains(Addr)) return false;
	switch (action) {
		case scinitialized:
		case sccode:
			return class_shared->initialized->contains(Addr);
		default:

			return true;
	}
}


