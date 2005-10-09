/*
 *	HT Editor
 *	htreg.cc
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
#include <stdlib.h>
#include <string.h>

#include "defreg.h"
#include "atom.h"
#include "htctrl.h"
#include "htdialog.h"
#include "htiobox.h"
#include "htreg.h"
#include "log.h"
#include "snprintf.h"
#include "store.h"
#include "tools.h"

ht_registry *registry;

#define ATOM_HT_REGISTRY		MAGIC32("REG\x00")
#define ATOM_HT_REGISTRY_NODE		MAGIC32("REG\x01")
#define ATOM_HT_REGISTRY_DATA_STREE	MAGIC32("REG\x02")
#define ATOM_HT_REGISTRY_DATA_DWORD	MAGIC32("REG\x03")
#define ATOM_HT_REGISTRY_DATA_RAW	MAGIC32("REG\x04")
#define ATOM_HT_REGISTRY_DATA_STRING	MAGIC32("REG\x05")
#define ATOM_HT_REGISTRY_NODE_TYPE_DESC	MAGIC32("REG\x10")
#define ATOM_HT_CREATE_EMPTY_SUBDIR	MAGIC32("REG\x20")
#define ATOM_HT_CREATE_EMPTY_SYMLINK	MAGIC32("REG\x21")
#define ATOM_HT_CREATE_EMPTY_DWORD	MAGIC32("REG\x22")
#define ATOM_HT_CREATE_EMPTY_STRING	MAGIC32("REG\x23")
#define ATOM_HT_CREATE_EMPTY_RAW	MAGIC32("REG\x24")

/*
 *	CLASS ht_registry_data
 */

bool ht_registry_data::editdialog(const char *keyname)
{
	return false;
}

void ht_registry_data::strvalue(char *buf32bytes)
{
	strcpy(buf32bytes, "<unknown>");
}

/*
 *	CLASS ht_registry_data_stree
 */

ht_registry_data_stree::ht_registry_data_stree(AVLTree *t)
{
	tree = t;
}

ht_registry_data_stree::~ht_registry_data_stree()
{
	delete tree;
}

void ht_registry_data_stree::load(ObjectStream &f)
{
	GET_OBJECT(f, tree);
}

ObjectID ht_registry_data_stree::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_STREE;
}

void ht_registry_data_stree::store(ObjectStream &f) const
{ 
	PUT_OBJECT(f, tree);
}

void ht_registry_data_stree::strvalue(char *buf32bytes)
{
	*buf32bytes = 0;
}

/*
 *	CLASS ht_registry_data_dword
 */

ht_registry_data_dword::ht_registry_data_dword(uint32 v)
{
	value = v;
}

bool ht_registry_data_dword::editdialog(const char *keyname)
{
// FIXME: use eval instead of strtol
	char result[32];
	ht_snprintf(result, sizeof result, "%d", value);
	if (inputbox("edit dword", "number", result, sizeof result, 0)==button_ok) {
		char *r;
		int i = (int)strtol(result, &r, 10);
		if (*r == 0) {
			value = i;
			return true;
		}
	}
	return false;
}

void ht_registry_data_dword::load(ObjectStream &f)
{
	GET_INT32X(f, value);
}

ObjectID ht_registry_data_dword::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_DWORD;
}

void ht_registry_data_dword::store(ObjectStream &f) const
{
	PUT_INT32X(f, value);
}

void ht_registry_data_dword::strvalue(char *buf32bytes)
{
	ht_snprintf(buf32bytes, 32, "%d", value);
}

/*
 *	CLASS ht_registry_data_raw
 */

ht_registry_data_raw::ht_registry_data_raw(const void *v, uint s)
{
	size = s;
	if (size) {
		value = malloc(s);
		memcpy(value, v, s);
	} else {
		value = 0;
	}
}

ht_registry_data_raw::~ht_registry_data_raw()
{
	if (value) free(value);
}

bool ht_registry_data_raw::editdialog(const char *keyname)
{
	bool r = false;
	Bounds b;
	b.w = 50;
	b.h = 15;
	b.x = (screen->w-b.w)/2;
	b.y = (screen->h-b.h)/2;
	
	ht_dialog *d = new ht_dialog();
	d->init(&b, "edit data_raw", FS_TITLE | FS_KILLER);
	
	if (d->run(false)) {
		r = true;
	}
	
	d->done();
	delete d;
	return r;
}

void ht_registry_data_raw::load(ObjectStream &f)
{
	GET_INT32D(f, size);
	GET_BINARY(f, value, size);
}

ObjectID ht_registry_data_raw::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_RAW;
}

void ht_registry_data_raw::store(ObjectStream &f) const
{
	PUT_INT32D(f, size);
	PUT_BINARY(f, value, size);
}

void ht_registry_data_raw::strvalue(char *buf32bytes)
{
	strcpy(buf32bytes, "raw");
}

/*
 *	CLASS ht_registry_data_string
 */
                                 
ht_registry_data_string::ht_registry_data_string(const char *s)
{
	value = ht_strdup(s);
}

ht_registry_data_string::~ht_registry_data_string()
{
	if (value) free(value);
}

bool ht_registry_data_string::editdialog(const char *keyname)
{
	char res[256];
	strncpy(res, value, 255);
	res[255] = 0;
	if (inputbox("edit string", "string", res, 256, 0)) {
		if (value) delete value;
		value = strdup(res);
		return true;
	}
	return false;
}

void ht_registry_data_string::load(ObjectStream &f)
{
	GET_STRING(f, value);
}

ObjectID ht_registry_data_string::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_STRING;
}

void ht_registry_data_string::store(ObjectStream &f) const
{
	PUT_STRING(f, value);
}

void ht_registry_data_string::strvalue(char *buf32bytes)
{
	strncpy(buf32bytes, value, 31);
	buf32bytes[31] = 0;
}

/*
 *	CLASS ht_registry_node
 */

ht_registry_node::ht_registry_node(ht_registry_node_type aType, const char *aName, ht_registry_data *aData)
{
	type = aType;
	name = ht_strdup(aName);
	data = aData;
}

ht_registry_node::~ht_registry_node()
{
	if (data) {
		data->done();
		delete data;
	}
	free(name);
}

void ht_registry_node::load(ObjectStream &f)
{
	GET_INT32X(f, type);
	GET_STRING(f, name);
	GET_OBJECT(f, data);
}

void ht_registry_node::store(ObjectStream &f) const
{
	PUT_INT32X(f, type);
	PUT_STRING(f, name);
	PUT_OBJECT(f, data);
}

ObjectID ht_registry_node::getObjectID() const
{
	return ATOM_HT_REGISTRY_NODE;
}

/*
 *	create_empty_*
 */

ht_registry_data *create_empty_symlink()
{
	return new ht_registry_data_string("");
}

ht_registry_data *create_empty_dword()
{
	return new ht_registry_data_dword(0);
}

ht_registry_data *create_empty_string()
{
	return new ht_registry_data_string("");
}

ht_registry_data *create_empty_raw()
{
	return new ht_registry_data_raw(NULL, 0);
}

/*
 *	CLASS ht_registry
 */
ht_registry_node_type_desc::ht_registry_node_type_desc(ht_registry_node_type aType, 
	const char *aName, create_empty_registry_data_func c)
{
	type = aType;
	name = ht_strdup(aName);
	create_empty_registry_data = c;
}

ht_registry_node_type_desc::~ht_registry_node_type_desc()
{
	free(name);
}

int ht_registry_node_type_desc::compareTo(const Object *d) const
{
	return strcmp(name, ((ht_registry_node_type_desc*)d)->name);
}

void ht_registry_node_type_desc::load(ObjectStream &f)
{
	GET_INT32D(f, type);
	GET_STRING(f, name);
	uint atom;
	GET_INT32X(f, atom);
	void *p = getAtomValue(atom);
	create_empty_registry_data=(create_empty_registry_data_func)p;
}

ObjectID ht_registry_node_type_desc::getObjectID() const
{
	return ATOM_HT_REGISTRY_NODE_TYPE_DESC;
}

void ht_registry_node_type_desc::store(ObjectStream &f) const
{
	PUT_INT32D(f, type);
	PUT_STRING(f, name);
	uint atom = getAtomId((void*)create_empty_registry_data);
	PUT_INT32X(f, atom);
}


void ht_registry::init()
{
// build registry root
	AVLTree *s = new AVLTree(true);
	root = new ht_registry_node(RNT_SUBDIR, "/", new ht_registry_data_stree(s));

// build node_types tree
	node_types = new AVLTree(true);

	struct bla {
		char *identifier;
		ht_registry_node_type type;
		create_empty_registry_data_func create_empty_registry_data;
	};
	bla b[]=
	{
		{"subdir", RNT_SUBDIR, NULL},
		{"symlink", RNT_SYMLINK, create_empty_symlink},
		{"dword", RNT_DWORD, create_empty_dword},
		{"string", RNT_STRING, create_empty_string},
		{"raw", RNT_RAW, create_empty_raw}
	};

	for (uint i=0; i<sizeof (b) / sizeof b[0]; i++) {
		ht_registry_node_type_desc *d=new ht_registry_node_type_desc(
			b[i].type, b[i].identifier, b[i].create_empty_registry_data);
		node_types->insert(d);
	}
}

void ht_registry::done()
{
	delete node_types;
	delete root;
}

int ht_registry::create_node(const char *key, ht_registry_node_type type)
{
	const char *name;
	ht_registry_node *m;
	if (!splitfind(key, &name, &m)) return EINVAL;

	if (!valid_nodename(name)) return EINVAL;

	ht_registry_node_type_desc *d = get_node_type_desc(type, NULL);
	if (d && d->create_empty_registry_data) {
		ht_registry_node t(0, name, NULL);
		if (((ht_registry_data_stree*)m->data)->tree->find(&t) != invObjHandle) return EEXIST;

		ht_registry_data *data = d->create_empty_registry_data();
		ht_registry_node *n=new ht_registry_node(type, name, data);
		((ht_registry_data_stree*)m->data)->tree->insert(n);
		return 0;
	}

	return ENOSYS;
}

int ht_registry::create_subdir(const char *key)
{
	const char *name;
	ht_registry_node *m;
	if (!splitfind(key, &name, &m)) return EINVAL;

	if (!valid_nodename(name)) return EINVAL;

	AVLTree *s=new AVLTree(true);

	ht_registry_node *n=new ht_registry_node(RNT_SUBDIR, name, new ht_registry_data_stree(s));
	bool ins;
	((ht_registry_data_stree*)m->data)->tree->findOrInsert(n, ins);
	if (!ins) {
		delete n;
		return EEXIST;
	}
	return 0;
}

int ht_registry::delete_node(const char *key)
{
	Container *dir;
	ht_registry_node *n = find_entry_i(&dir, key, 0);
	if (!n) return ENOENT;
	const char *s = strrchr(key, '/');
	if (s) s++; else s=key;
	ht_registry_node ss(0, s, NULL);
	return dir->delObj(&ss) ? 0 : ENOENT;
}

void ht_registry::debug_dump()
{
#if 0
//	FILE *f=fopen("", "");
	debug_dump_i(stderr, ((ht_registry_data_stree*)root->data)->tree, 0);
//	fclose(f);
#endif
}

void ht_registry::debug_dump_i(FILE *f, Container *t, int ident)
{
#if 0
	ht_data_string *key=NULL;
	ht_registry_node *n;
	while ((key=(ht_data_string*)t->enum_next((ht_data**)&n, key))) {
		for (int i=0; i<ident; i++) fprintf(f, "     ");
		fprintf(f, "%s ", key->value);
		switch (n->type) {
			case RNT_DWORD:
				fprintf(f, "= (uint32) %08d (%08x)\n", ((ht_data_dword*)n->data)->value, ((ht_data_dword*)n->data)->value);
				break;
			case RNT_STRING:
				fprintf(f, "= (string) \"%s\"\n", ((ht_data_string*)n->data)->value);
				break;
			case RNT_SYMLINK:
				fprintf(f, "=> \"%s\"\n", ((ht_data_string*)n->data)->value);
				break;
			case RNT_SUBDIR:
				fprintf(f, "{\n");
				debug_dump_i(f, ((ht_registry_data_stree*)n->data)->tree, ident+1);
				for (int i=0; i<ident; i++) fprintf(f, "     ");
				fprintf(f, "}\n");
				break;
			case RNT_RAW:
				fprintf(f, "= (raw) nyi!\n");
				break;
			default: {
				char *name=lookup_node_type_name(n->type);
				if (!name) name="?";
				fprintf(f, "= ('%s'=%d)\n", name, n->type);
				break;
			}
		}
	}
#endif
}

ht_registry_node *ht_registry::enum_next(const char *dir, ht_registry_node *prevkey)
{
	ht_registry_node *n = find_entry_i(0, dir, 1);
	if (n) {
		if (n->type != RNT_SUBDIR) return NULL;
		Container *t = ((ht_registry_data_stree*)n->data)->tree;

		if (prevkey) {
			return (ht_registry_node *)t->get(t->findG(prevkey));
		} else {
			return (ht_registry_node *)t->get(t->findFirst());
		}
	}
	return NULL;
}

ht_registry_node *ht_registry::enum_prev(const char *dir, ht_registry_node *prevkey)
{
	ht_registry_node *n = find_entry_i(0, dir, 1);
	if (n) {
		if (n->type != RNT_SUBDIR) return NULL;
		Container *t = ((ht_registry_data_stree*)n->data)->tree;

		if (prevkey) {
			return (ht_registry_node *)t->get(t->findL(prevkey));
		} else {
			return (ht_registry_node *)t->get(t->findFirst());
		}
	}
	return NULL;
}

bool ht_registry::find_any_entry(const char *key, ht_registry_node **node)
{
	ht_registry_node *n=find_entry_i(0, key, 1);
	if (n) {
		*node = n;
		return true;
	}
	return false;
}

bool ht_registry::find_data_entry(const char *key, ht_registry_node **node, bool follow_symlinks)
{
	ht_registry_node *n=find_entry_i(0, key, follow_symlinks);
	if (n) {
		if (n->type == RNT_SUBDIR) return false;
		*node = n;
		return true;
	}
	return false;
}

ht_registry_node *ht_registry::find_entry_i(Container **rdir, const char *key, bool follow_symlinks)
{
	ht_registry_node *dir = root;
	char *s;
	char t[256]; /* FIXME: possible buffer overflow */
	if (key[0]=='/') key++;
	while (1) {
		s = strchr(key, '/');
		if (s) {
			strncpy(t, key, s-key);
			t[s-key] = 0;
			dir = find_entry_get_subdir(((ht_registry_data_stree*)dir->data)->tree, t);
			if (!dir) break;
			key = s+1;
		} else {
			ht_registry_node *n;
			if (*key==0) {
				n=dir;
			} else {
				n=find_entry_get_data(((ht_registry_data_stree*)dir->data)->tree, key, follow_symlinks);
			}
			if (rdir) *rdir=((ht_registry_data_stree*)dir->data)->tree;
			return n;
		}
	}
	return NULL;
}

ht_registry_node *ht_registry::find_entry_get_node(Container *dir, const char *nodename)
{
	if (nodename) {
		ht_registry_node t(0, nodename, NULL);
		ht_registry_node *n=(ht_registry_node*)dir->get(&t);
		return n;
	}
	return NULL;
}

ht_registry_node *ht_registry::find_entry_get_subdir(Container *dir, const char *nodename)
{
	ht_registry_node *n=find_entry_get_node(dir, nodename);
start:
	if (!n) return 0;
	switch (n->type) {
		case RNT_SYMLINK: {
			rec_depth++;
			if (rec_depth>MAX_SYMLINK_REC_DEPTH) return 0;
			char *sl=((ht_registry_data_string*)n->data)->value;
			if (sl[0]=='/') {
				n=find_entry_i(0, sl, 1);
				goto start;
			} else {
				return find_entry_get_subdir(dir, sl);
			}
		}
		case RNT_SUBDIR:
			return n;
	}
	return 0;
}

ht_registry_node *ht_registry::find_entry_get_data(Container *dir, const char *nodename, bool follow_symlinks)
{
	ht_registry_node *n = find_entry_get_node(dir, nodename);
start:
	if (!n) return 0;
	if ((follow_symlinks) && (n->type==RNT_SYMLINK)) {
		rec_depth++;
		if (rec_depth > MAX_SYMLINK_REC_DEPTH) return 0;
		char *sl = ((ht_registry_data_string*)n->data)->value;
		if (sl[0]=='/') {
			n = find_entry_i(0, sl, 1);
			goto start;
		} else {
			return find_entry_get_data(dir, sl, follow_symlinks);
		}
	}
	return n;
}

ht_registry_node_type ht_registry::have_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data)
{
	ht_registry_node_type t=lookup_node_type(identifier);
	if (!t) t=register_node_type(identifier, create_empty_registry_data);
	return t;
}

void ht_registry::load(ObjectStream &f)
{
	GET_OBJECT(f, node_types);
	GET_OBJECT(f, root);
}

ht_registry_node_type_desc *ht_registry::get_node_type_desc(ht_registry_node_type t, char **identifier)
{
	ht_registry_node_type_desc *data = NULL;
	firstThat(ht_registry_node_type_desc, data, *node_types, t == data->type);
	if (data && identifier) *identifier = data->name;
	return data;
}

ht_registry_node_type ht_registry::lookup_node_type(const char *identifier)
{
	ht_registry_node_type_desc s(0, identifier, NULL);
	ht_registry_node_type_desc *d=(ht_registry_node_type_desc*)node_types->get(&s);
	return d ? d->type : 0;
}

ObjectID ht_registry::getObjectID() const
{
	return ATOM_HT_REGISTRY;
}

ht_registry_node_type ht_registry::register_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data)
{
//	ht_registry_node_type t = RNT_USER;
	ht_registry_node_type t = 0;
	ht_registry_node_type_desc *nt = NULL;
	do {
		t++;
		firstThat(ht_registry_node_type_desc, nt, *node_types, t == nt->type);
	} while (nt != NULL);

	ht_registry_node_type_desc *v = new ht_registry_node_type_desc(t, 
		identifier, create_empty_registry_data);

	if (node_types->get(v) != invObjHandle) {
		delete v;
		return RNT_INVALID;
	} else {
		node_types->insert(v);
		return t;
	}
}

int ht_registry::set_dword(const char *key, uint32 d)
{
	return set_node(key, RNT_DWORD, new ht_registry_data_dword(d));
}

int ht_registry::set_raw(const char *key, const void *data, uint size)
{
	return set_node(key, RNT_RAW, new ht_registry_data_raw(data, size));
}

int ht_registry::set_node(const char *key, ht_registry_node_type type, ht_registry_data *data)
{
	ht_registry_node *n = find_entry_i(0, key, 0);
	if (!n) return ENOENT;

	if (n->type == type) {
		if (n->data) {
			n->data->done();
			delete n->data;
		}
		n->data = data;
		return 0;
	}
	return EPERM;
}

int ht_registry::set_string(const char *key, const char *string)
{
	return set_node(key, RNT_STRING, new ht_registry_data_string(string));
}

int ht_registry::set_symlink(const char *key, const char *dest)
{
	return set_node(key, RNT_SYMLINK, new ht_registry_data_string(dest));
}

bool ht_registry::splitfind(const char *key, const char **name, ht_registry_node **node)
{
	char dir[256]; /* FIXME: possible buffer overflow */
	const char *n=strrchr(key, '/');
	if (n) {
		strncpy(dir, key, n-key);
		dir[n-key]=0;
		n++;
	} else {
		dir[0]=0;
		n=key;
	}

	ht_registry_node *m=find_entry_i(0, dir, 1);
	if (!m) return 0;
	if (m->type!=RNT_SUBDIR) return 0;
	*node=m;
	*name=n;
	return 1;
}

void ht_registry::store(ObjectStream &f) const
{
	PUT_OBJECT(f, node_types);
	PUT_OBJECT(f, root);
}

unsigned char valid_nodename_chars[256/8]=
{
/* 00, 08, 10, 18 */
	0, 0, 0, 0,
/* 20, 28, 30, 38 */
/* space '-' 0-9 '&' ':' '.' */
	BITMAP(1, 0, 0, 0, 0, 0, 1, 0), BITMAP(0, 0, 0, 0, 0, 1, 1, 0),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 0, 0, 0, 0, 0),
/* 40, 48, 50, 58 */
/* A-Z '_' */
	BITMAP(0, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 1, 1, 1, 1, 1),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 0, 0, 0, 0, 1),
/* 60, 68, 70, 78 */
/* a-z */
	BITMAP(0, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 1, 1, 1, 1, 1),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 0, 0, 0, 0, 0),
/* 80, 88, 90, 98 */
	0, 0, 0, 0,
/* a0, a8, b0, b8 */
	0, 0, 0, 0,
/* c0, c8, d0, d8 */
	0, 0, 0, 0,
/* e0, e8, f0, f8 */
	0, 0, 0, 0
};

unsigned char valid_nodename_chars_first[256/8]=
{
/* 00, 08, 10, 18 */
	0, 0, 0, 0,
/* 20, 28, 30, 38 */
/* '.' '-' 0-9 */
	BITMAP(0, 0, 0, 0, 0, 0, 1, 0), BITMAP(0, 0, 0, 0, 0, 1, 1, 0),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 0, 0, 0, 0, 0, 0),
/* 40, 48, 50, 58 */
/* A-Z '_' */
	BITMAP(0, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 1, 1, 1, 1, 1),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 0, 0, 0, 0, 1),
/* 60, 68, 70, 78 */
/* a-z */
	BITMAP(0, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 1, 1, 1, 1, 1),
	BITMAP(1, 1, 1, 1, 1, 1, 1, 1), BITMAP(1, 1, 1, 0, 0, 0, 0, 0),
/* 80, 88, 90, 98 */
	0, 0, 0, 0,
/* a0, a8, b0, b8 */
	0, 0, 0, 0,
/* c0, c8, d0, d8 */
	0, 0, 0, 0,
/* e0, e8, f0, f8 */
	0, 0, 0, 0
};

bool valid_char(unsigned char *bitmap, char c)
{
	int o=c/8;
	int p=c%8;
	return BITBIT(bitmap[o], p);
}

bool ht_registry::valid_nodename(const char *nodename)
{
	if ((strcmp(nodename, "..")==0) || (strcmp(nodename, ".")==0)) {
		return false;
	}
	if (!valid_char(valid_nodename_chars_first, *nodename)) return false;
	nodename++;
	while (*nodename) {
		if (!valid_char(valid_nodename_chars, *nodename)) return false;
		nodename++;
	}
	return true;
}

uint32 get_config_dword(char *ident)
{
	char e[HT_NAME_MAX], *ee = e;
	strcpy(ee, "/config/"); ee += strlen(ee);
	strncpy(ee, ident, sizeof (e) - (ee-e));
	ht_registry_node *n;
	if (registry->find_data_entry(e, &n, true)) {
		if (n->type == RNT_DWORD) {
			ht_registry_data_dword *s = (ht_registry_data_dword *)n->data;
			return s->value;
		} else {
			char *q = "?";
			registry->get_node_type_desc(n->type, &q);
			LOG_EX(LOG_ERROR, "registry key '%s' not of type %s, but: %s", e, "dword", q);
		}
	} else LOG_EX(LOG_ERROR, "registry key '%s' not found", e);
	return 0;
}

char *get_config_string(char *ident)
{
	char e[HT_NAME_MAX], *ee = e;
	strcpy(ee, "/config/"); ee += strlen(ee);
	strncpy(ee, ident, sizeof (e) - (ee-e));
	ht_registry_node *n;
	if (registry->find_data_entry(e, &n, true)) {
		if (n->type == RNT_STRING) {
			ht_registry_data_string *s = (ht_registry_data_string *)n->data;
			return ht_strdup(s->value);
		} else {
			char *q = "?";
			registry->get_node_type_desc(n->type, &q);
			LOG_EX(LOG_ERROR, "registry key '%s' not of type %s, but: %s", e, "string", q);
		}
	} else LOG_EX(LOG_ERROR, "registry key '%s' not found", e);
	return NULL;
}

BUILDER(ATOM_HT_REGISTRY, ht_registry, Object);
BUILDER(ATOM_HT_REGISTRY_NODE, ht_registry_node, Object);
BUILDER(ATOM_HT_REGISTRY_DATA_STREE, ht_registry_data_stree, ht_registry_data);
BUILDER(ATOM_HT_REGISTRY_DATA_DWORD, ht_registry_data_dword, ht_registry_data);
BUILDER(ATOM_HT_REGISTRY_DATA_RAW, ht_registry_data_raw, ht_registry_data);
BUILDER(ATOM_HT_REGISTRY_DATA_STRING, ht_registry_data_string, ht_registry_data);
BUILDER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc, Object);
	
/*
 *	INIT
 */

bool init_registry()
{
	REGISTER(ATOM_HT_REGISTRY, ht_registry);
	REGISTER(ATOM_HT_REGISTRY_NODE, ht_registry_node);
	REGISTER(ATOM_HT_REGISTRY_DATA_STREE, ht_registry_data_stree);
	REGISTER(ATOM_HT_REGISTRY_DATA_DWORD, ht_registry_data_dword);
	REGISTER(ATOM_HT_REGISTRY_DATA_RAW, ht_registry_data_raw);
	REGISTER(ATOM_HT_REGISTRY_DATA_STRING, ht_registry_data_string);
	REGISTER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc);
//	registerAtom(ATOM_HT_CREATE_EMPTY_SUBDIR, (void*));
	registerAtom(ATOM_HT_CREATE_EMPTY_SYMLINK, (void*)create_empty_symlink);
	registerAtom(ATOM_HT_CREATE_EMPTY_DWORD, (void*)create_empty_dword);
	registerAtom(ATOM_HT_CREATE_EMPTY_STRING, (void*)create_empty_string);
	registerAtom(ATOM_HT_CREATE_EMPTY_RAW, (void*)create_empty_raw);

	/*
	 *	load default registry
	 */
	ConstMemMapFile f(default_reg, sizeof default_reg);
	ObjectStreamBin o(&f, false);

	GET_OBJECT(o, registry);

	return true;
}

/*
 *	DONE
 */

void done_registry()
{
	UNREGISTER(ATOM_HT_REGISTRY, ht_registry);
	UNREGISTER(ATOM_HT_REGISTRY_NODE, ht_registry_node);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_STREE, ht_registry_data_stree);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_DWORD, ht_registry_data_dword);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_RAW, ht_registry_data_raw);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_STRING, ht_registry_data_string);
	UNREGISTER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc);
//	unregisterAtom(ATOM_HT_CREATE_EMPTY_SUBDIR);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_SYMLINK);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_DWORD);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_STRING);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_RAW);
	
	registry->done();
	delete registry;
}

