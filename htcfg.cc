/*
 *	HT Editor
 *	htcfg.cc
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
 
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "cstream.h"
#include "htcfg.h"
#include "htctrl.h"
#include "htdebug.h"
#include "endianess.h"
#include "htreg.h"
#include "strtools.h"
#include "stream.h"
#include "store.h"
#include "sys.h"
#include "tools.h"

/* VERSION 2 (for ht-0.4.4 and later) */

/* NOTE: as of Version 2 ALL integers in HT config-files are
   stored in big-endian format... (non-intel) */
   
#define object_stream_bin			0
#define object_stream_txt			1
#define object_stream_bin_compressed		2

ObjectStream *create_object_stream(Stream &f, int object_stream_type)
{
	ObjectStream *s;
	switch (object_stream_type) {
		case object_stream_bin: {
			s = new ObjectStreamBin(&f, false);
			break;
		}
		case object_stream_txt: {
			s = new ObjectStreamText(&f, false);
			break;
		}
		case object_stream_bin_compressed: {
			ht_compressed_stream *cs=new ht_compressed_stream(&f, false);
			s = new ObjectStreamBin(cs, true);
			break;
		}
		default: {
			return NULL;
		}
	}
	return s;
}

struct config_header {
	char magic[4] PACKED;
	char version[4] PACKED;
	char stream_type[2] PACKED;
};

/*
 *	system configs
 */

char *systemconfig_file;

/**/

loadstore_result save_systemconfig()
{
	try {
		LocalFile f((String)systemconfig_file, IOAM_WRITE, FOM_CREATE);
	
		/* write project config header */
		config_header h;

		memcpy(h.magic, ht_systemconfig_magic, sizeof h.magic);

		char q[16];

		int system_ostream_type = get_config_dword("misc/config format");
	        system_ostream_type =object_stream_txt;

		sprintf(q, "%04x", ht_systemconfig_fileversion);
		memcpy(h.version, q, sizeof h.version);

		sprintf(q, "%02x", system_ostream_type);
		memcpy(h.stream_type, q, sizeof h.stream_type);

		f.writex(&h, sizeof h);
	
		/* write object stream type */
		std::auto_ptr<ObjectStream> d(create_object_stream(f, system_ostream_type));
	   
		switch (system_ostream_type) {
		case object_stream_bin:
			break;
		case object_stream_txt:
			f.writex((void*)"\n#\n#\tThis is a generated file!\n#\n", 33);
			break;
		}
		/* write config */
		app->store(*d.get());
	} catch (const IOException &) {
		return LS_ERROR_WRITE;
	}
	return LS_OK;
}

bool load_systemconfig(loadstore_result *result, int *error_info)
{
	uint8 object_stream_type = 128;
	std::auto_ptr<ObjectStream> d(NULL);
	*error_info = 0;
	try {
		LocalFile f((String)systemconfig_file, IOAM_READ, FOM_EXISTS);
		/* read project config header */
		config_header h;

		if (f.read(&h, sizeof h) != sizeof h
		 || memcmp(h.magic, ht_systemconfig_magic, sizeof h.magic) != 0) {
			*result = LS_ERROR_MAGIC;
			return false;
		}
	

		uint16 readver;
		if (!hexw_ex(readver, (char*)h.version) || (readver != ht_systemconfig_fileversion)) {
			*result = LS_ERROR_VERSION;
			*error_info = readver;
			return false;
		}

		/* object stream type */
		if (!hexb_ex(object_stream_type, (char*)h.stream_type)) {
			*result = LS_ERROR_FORMAT;
			return false;
		}

		d.reset(create_object_stream(f, object_stream_type));
		if (!d.get()) {
			*result = LS_ERROR_FORMAT;
			return false;
		}

		/* read config */
		app->load(*d.get());
	} catch (const ObjectNotRegisteredException &) {
		*result = LS_ERROR_CORRUPTED;
		if (object_stream_type==object_stream_txt && d.get()) {
			*error_info = ((ObjectStreamText*)d.get())->getErrorLine();
		}
		return false;
	} catch (const IOException &e) {
		if (e.mPosixErrno == ENOENT) {
			*result = LS_ERROR_NOT_FOUND;
		} else {
			*result = LS_ERROR_READ;
			if (object_stream_type == object_stream_txt && d.get()) {
				*error_info = ((ObjectStreamText*)d.get())->getErrorLine();
			}
		}
		return false;
	}
	*result = LS_OK;
	return true;
}

/**/

loadstore_result save_fileconfig(const char *fileconfig_file, const char *magic, uint version, store_fcfg_func store_func, void *context)
{
	try {
		LocalFile f((String)systemconfig_file, IOAM_WRITE, FOM_CREATE);
	
		/* write file config header */
		config_header h;

		memcpy(h.magic, magic, sizeof h.magic);

		char q[16];

		int file_ostream_type = get_config_dword("misc/config format");
	
		sprintf(q, "%04x", version);
		memcpy(h.version, q, sizeof h.version);

		sprintf(q, "%02x", file_ostream_type);
		memcpy(h.stream_type, q, sizeof h.stream_type);

		f.writex(&h, sizeof h);

		/* object stream type */
		std::auto_ptr<ObjectStream> d(create_object_stream(f, file_ostream_type));
	   
		switch (file_ostream_type) {
		case object_stream_bin:
			break;
		case object_stream_txt:
			f.writex((void*)"\n#\n#\tThis is a generated file!\n#\n", 33);
			break;
		}
		/* write config */
		store_func(*d.get(), context);
	} catch (const IOException &) {
		return LS_ERROR_WRITE;
	}
	return LS_OK;
}

loadstore_result load_fileconfig(const char *fileconfig_file, const char *magic, uint version, load_fcfg_func load_func, void *context, int *error_info)
{
	uint8 object_stream_type = 128;
	*error_info = 0;
	std::auto_ptr<ObjectStream> d(NULL);
	try {
		LocalFile f((String)fileconfig_file, IOAM_READ, FOM_EXISTS);
		/* read file config header */
		config_header h;

		if (f.read(&h, sizeof h) != sizeof h 
		 || memcmp(h.magic, magic, sizeof h.magic) != 0) {
			return LS_ERROR_MAGIC;
		}
	

		uint16 readver;
		if (!hexw_ex(readver, (char*)h.version) || (readver != version)) {
			*error_info = readver;
			return LS_ERROR_VERSION;
		}
	
		if (!hexb_ex(object_stream_type, (char*)h.stream_type)) {
			return LS_ERROR_FORMAT;
		}

		/* object stream type */
		d.reset(create_object_stream(f, object_stream_type));

		if (!d.get()) {
			return LS_ERROR_FORMAT;
		}		
	   
		load_func(*d.get(), context);
		
	} catch (const ObjectNotRegisteredException &) {
		if (object_stream_type==object_stream_txt && d.get()) {
			*error_info = ((ObjectStreamText*)d.get())->getErrorLine();
		}
		return LS_ERROR_CORRUPTED;
	} catch (const IOException &e) {
		if (e.mPosixErrno == ENOENT) {
			return LS_ERROR_NOT_FOUND;
		} else {
			if (object_stream_type==object_stream_txt && d.get()) {
				*error_info = ((ObjectStreamText*)d.get())->getErrorLine();
			}
        		return LS_ERROR_READ;
		}
	}
	return LS_OK;
}

/*
 *	INIT
 */

bool init_cfg()
{
#if defined(MSDOS) || defined(DJGPP) || defined(WIN32) || defined(__WIN32__)
	char d[1024];	/* FIXME: !!!! */
	sys_dirname(d, appname);
	char *b = "\\"SYSTEM_CONFIG_FILE_NAME;
	systemconfig_file = ht_malloc(strlen(d)+strlen(b)+1);
	strcpy(systemconfig_file, d);
	strcat(systemconfig_file, b);
#else
	char *home = getenv("HOME");
	char *b = "/"SYSTEM_CONFIG_FILE_NAME;
	if (!home) home = "";
	systemconfig_file = ht_malloc(strlen(home)+strlen(b)+1);
	strcpy(systemconfig_file, home);
	strcat(systemconfig_file, b);
#endif
	return true;
}

/*
 *	DONE
 */

void done_cfg()
{
	free(systemconfig_file);
}
