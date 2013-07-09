/* 
 *	HT Editor
 *	htreg.h
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

#ifndef __HTREG_H__
#define __HTREG_H__

#include "data.h"
#include "strtools.h"

/*
 *	CLASS RegistryData
 */

class RegistryData: public Object {
public:
/* new */
		RegistryData() {};
		RegistryData(BuildCtorArg&a): Object(a) {};
	virtual	bool editdialog(const char *keyname);
	virtual void strvalue(char *buf32bytes);
};

/*
 *	CLASS RegistryDataSTree
 */

class RegistryDataSTree: public RegistryData {
public:
	Container *tree;

			RegistryDataSTree(AVLTree *aTree);
			RegistryDataSTree(BuildCtorArg&a): RegistryData(a) {};
			~RegistryDataSTree();
/* overwritten */
	virtual	void load(ObjectStream &f);
	virtual	ObjectID getObjectID() const;
	virtual	void store(ObjectStream &f) const;
	virtual	void strvalue(char *buf32bytes);
};

/*
 *	CLASS RegistryDataDword
 */

class RegistryDataDword: public RegistryData {
public:
	uint32 value;

		RegistryDataDword(uint32 value);
		RegistryDataDword(BuildCtorArg&a): RegistryData(a) {};
/* overwritten */
	virtual	bool editdialog(const char *keyname);
	virtual	void load(ObjectStream &f);
	virtual	ObjectID getObjectID() const;
	virtual	void store(ObjectStream &f) const;
	virtual	void strvalue(char *buf32bytes);
};

/*
 *	CLASS RegistryDataRaw
 */

class RegistryDataRaw: public RegistryData {
public:
	void *value;
	uint size;

			RegistryDataRaw(const void *value, uint size);
			RegistryDataRaw(BuildCtorArg&a): RegistryData(a) {};
			~RegistryDataRaw();
/* overwritten */
	virtual	bool editdialog(const char *keyname);
	virtual	void load(ObjectStream &f);
	virtual	ObjectID getObjectID() const;
	virtual	void store(ObjectStream &f) const;
	virtual	void strvalue(char *buf32bytes);
};

/*
 *	CLASS RegistryDataString
 */

class RegistryDataString: public RegistryData {
public:
	char *value;

		RegistryDataString(const char *s);
		RegistryDataString(BuildCtorArg&a): RegistryData(a) {};
		~RegistryDataString();
/* overwritten */
	virtual	bool editdialog(const char *keyname);
	virtual	void load(ObjectStream &f);
	virtual	ObjectID getObjectID() const;
	virtual	void store(ObjectStream &f) const;
	virtual	void strvalue(char *buf32bytes);
};

typedef RegistryData* (*create_empty_registry_data_func)();

typedef uint ht_registry_node_type;

class ht_registry_node_type_desc: public Object {
public:
	char *name;
	ht_registry_node_type type;
	create_empty_registry_data_func create_empty_registry_data;
	
	ht_registry_node_type_desc(ht_registry_node_type t, const char *name, create_empty_registry_data_func c);
	ht_registry_node_type_desc(BuildCtorArg&a): Object(a) {};
	virtual ~ht_registry_node_type_desc();
	virtual int compareTo(const Object *) const;
	virtual	void load(ObjectStream &f);
	virtual	ObjectID getObjectID() const;
	virtual	void store(ObjectStream &f) const;
};

#define RNT_INVALID		0	/* returned by some functions */
// these are predefined
#define RNT_SUBDIR		1
#define RNT_SYMLINK		2
#define RNT_DWORD  		3
#define RNT_STRING 		4
#define RNT_RAW		5

#define RNT_USER    	0x100
// the rest may be allocated dynamically

/*
 *	CLASS RegistryNode
 */

class RegistryNode: public Object {
public:
	char *name;
	ht_registry_node_type type;
	RegistryData *data;

	RegistryNode(ht_registry_node_type type, const char *name, RegistryData *data);
	RegistryNode(BuildCtorArg&a): Object(a) {};
	virtual ~RegistryNode();
/* overwritten */
	virtual int compareTo(const Object *) const;
	virtual	void load(ObjectStream &f);
	virtual	void store(ObjectStream &f) const;
	virtual	ObjectID getObjectID() const;
};

/*
 *	CLASS Registry
 */

#define MAX_SYMLINK_REC_DEPTH 20

class Registry: public Object {
protected:
	RegistryNode *root;
	uint rec_depth;

			RegistryNode *find_entry_i(Container **dir, const char *key, bool follow_symlinks);
			RegistryNode *find_entry_get_node(Container *dir, const char *nodename);
			RegistryNode *find_entry_get_subdir(Container *dir, const char *nodename);
			RegistryNode *find_entry_get_data(Container *dir, const char *nodename, bool follow_symlinks);
			bool splitfind(const char *key, const char **name, RegistryNode **node);
public:
	Container *node_types;

		Registry() {};
		Registry(BuildCtorArg&a): Object(a) {};
		void init();
	virtual	void done();
/* new */
		int create_node(const char *key, ht_registry_node_type type);
		int create_subdir(const char *key);
		int delete_node(const char *key);
		RegistryNode *enum_next(const char *dir, RegistryNode *prevkey);
		RegistryNode *enum_prev(const char *dir, RegistryNode *nextkey);
			
		bool find_any_entry(const char *key, RegistryNode **node);
		bool find_data_entry(const char *key, RegistryNode **node, bool follow_symlinks, Container **dir = NULL);
		/* node type*/
		ht_registry_node_type lookup_node_type(const char *identifier);
		ht_registry_node_type_desc *get_node_type_desc(ht_registry_node_type t, const char **identifier);
		ht_registry_node_type have_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data);
		ht_registry_node_type register_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data);
		/**/
		int set_dword(const char *key, uint32 d);
		int set_raw(const char *key, const void *data, uint size);
		int set_node(const char *key, ht_registry_node_type type, RegistryData *data);
		int set_string(const char *key, const char *string);
		int set_symlink(const char *key, const char *dest);
		bool valid_nodename(const char *nodename);
/* overwritten */
	virtual	void load(ObjectStream &f);
	virtual	void store(ObjectStream &f) const;
	virtual	ObjectID getObjectID() const;
/* debug */
			void debug_dump();
			void debug_dump_i(FILE *f, Container *t, int ident);
};

uint32 get_config_dword(const char *ident, uint32 default_value=0);
char *get_config_string(const char *ident);

extern Registry *registry;

/*
 *	INIT
 */

bool init_registry();

/*
 *	DONE
 */

void done_registry();

#endif /* __HTREG_H__ */
