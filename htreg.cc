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
#include "htpal.h"
#include "htreg.h"
#include "log.h"
#include "snprintf.h"
#include "store.h"
#include "tools.h"

Registry *registry;

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

#define GENERATE_NEW_REGISTRY 0

/*
 *	CLASS RegistryData
 */

bool RegistryData::editdialog(const char *keyname)
{
	return false;
}

void RegistryData::strvalue(char *buf32bytes)
{
	strcpy(buf32bytes, "<unknown>");
}

/*
 *	CLASS RegistryDataSTree
 */

RegistryDataSTree::RegistryDataSTree(AVLTree *t)
{
	tree = t;
}

RegistryDataSTree::~RegistryDataSTree()
{
	delete tree;
}

void RegistryDataSTree::load(ObjectStream &f)
{
	GET_OBJECT(f, tree);
}

ObjectID RegistryDataSTree::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_STREE;
}

void RegistryDataSTree::store(ObjectStream &f) const
{
	PUT_OBJECT(f, tree);
}

void RegistryDataSTree::strvalue(char *buf32bytes)
{
	*buf32bytes = 0;
}

/*
 *	CLASS RegistryDataDword
 */

RegistryDataDword::RegistryDataDword(uint32 v)
{
	value = v;
}

bool RegistryDataDword::editdialog(const char *keyname)
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

void RegistryDataDword::load(ObjectStream &f)
{
	GET_INT32X(f, value);
}

ObjectID RegistryDataDword::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_DWORD;
}

void RegistryDataDword::store(ObjectStream &f) const
{
	PUT_INT32X(f, value);
}

void RegistryDataDword::strvalue(char *buf32bytes)
{
	ht_snprintf(buf32bytes, 32, "%d", value);
}

/*
 *	CLASS RegistryDataRaw
 */

RegistryDataRaw::RegistryDataRaw(const void *v, uint s)
{
	size = s;
	if (size) {
		value = malloc(s);
		memcpy(value, v, s);
	} else {
		value = 0;
	}
}

RegistryDataRaw::~RegistryDataRaw()
{
	free(value);
}

bool RegistryDataRaw::editdialog(const char *keyname)
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

void RegistryDataRaw::load(ObjectStream &f)
{
	GET_INT32D(f, size);
	GET_BINARY(f, value, size);
}

ObjectID RegistryDataRaw::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_RAW;
}

void RegistryDataRaw::store(ObjectStream &f) const
{
	PUT_INT32D(f, size);
	PUT_BINARY(f, value, size);
}

void RegistryDataRaw::strvalue(char *buf32bytes)
{
	strcpy(buf32bytes, "raw");
}

/*
 *	CLASS RegistryDataString
 */

RegistryDataString::RegistryDataString(const char *s)
{
	value = ht_strdup(s);
}

RegistryDataString::~RegistryDataString()
{
	free(value);
}

bool RegistryDataString::editdialog(const char *keyname)
{
	char res[256];
	ht_strlcpy(res, value, sizeof res);
	if (inputbox("edit string", "string", res, sizeof res, 0)) {
		free(value);
		value = strdup(res);
		return true;
	}
	return false;
}

void RegistryDataString::load(ObjectStream &f)
{
	GET_STRING(f, value);
}

ObjectID RegistryDataString::getObjectID() const
{
	return ATOM_HT_REGISTRY_DATA_STRING;
}

void RegistryDataString::store(ObjectStream &f) const
{
	PUT_STRING(f, value);
}

void RegistryDataString::strvalue(char *buf32bytes)
{
	ht_strlcpy(buf32bytes, value, 32);
}

/*
 *	CLASS RegistryNode
 */

RegistryNode::RegistryNode(ht_registry_node_type aType, const char *aName, RegistryData *aData)
{
	type = aType;
	name = ht_strdup(aName);
	data = aData;
}

RegistryNode::~RegistryNode()
{
	if (data) {
		data->done();
		delete data;
	}
	free(name);
}

int RegistryNode::compareTo(const Object *o) const
{
	return strcmp(name, ((RegistryNode*)o)->name);
}

void RegistryNode::load(ObjectStream &f)
{
	GET_INT32X(f, type);
	GET_STRING(f, name);
	GET_OBJECT(f, data);
}

void RegistryNode::store(ObjectStream &f) const
{
	PUT_INT32X(f, type);
	PUT_STRING(f, name);
	PUT_OBJECT(f, data);
}

ObjectID RegistryNode::getObjectID() const
{
	return ATOM_HT_REGISTRY_NODE;
}

/*
 *	create_empty_*
 */

static RegistryData *create_empty_symlink()
{
	return new RegistryDataString("");
}

static RegistryData *create_empty_dword()
{
	return new RegistryDataDword(0);
}

static RegistryData *create_empty_string()
{
	return new RegistryDataString("");
}

static RegistryData *create_empty_raw()
{
	return new RegistryDataRaw(NULL, 0);
}

/*
 *	CLASS Registry
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


RegistryData *create_empty_palette_entry();
void Registry::init()
{
#if GENERATE_NEW_REGISTRY
	// build registry root
	AVLTree *s = new AVLTree(true);
	root = new RegistryNode(RNT_SUBDIR, "/", new RegistryDataSTree(s));
	AVLTree *config = new AVLTree(true);
	AVLTree *palette = new AVLTree(true);
	s->insert(new RegistryNode(RNT_SUBDIR, "config", new RegistryDataSTree(config)));
	s->insert(new RegistryNode(RNT_SUBDIR, "palette", new RegistryDataSTree(palette)));
	AVLTree *config_misc = new AVLTree(true);
	AVLTree *config_editor = new AVLTree(true);
	config->insert(new RegistryNode(RNT_SUBDIR, "misc", new RegistryDataSTree(config_misc)));
	config_misc->insert(new RegistryNode(RNT_DWORD, "config format", new RegistryDataDword(2)));
	config_misc->insert(new RegistryNode(RNT_STRING, "statusline", new RegistryDataString("%a %L %t %d")));
	config_misc->insert(new RegistryNode(RNT_STRING, "vfs display format", new RegistryDataString("type,name:20+,|,desc,|,bsize,|,perm,|,rmtime")));
	config->insert(new RegistryNode(RNT_SUBDIR, "editor", new RegistryDataSTree(config_editor)));
	config_editor->insert(new RegistryNode(RNT_STRING, "EOF", new RegistryDataString("<EOF>")));
	config_editor->insert(new RegistryNode(RNT_STRING, "EOL", new RegistryDataString(".")));
	config_editor->insert(new RegistryNode(RNT_DWORD, "auto indent", new RegistryDataDword(1)));
	config_editor->insert(new RegistryNode(RNT_DWORD, "scroll offset", new RegistryDataDword(3)));
	config_editor->insert(new RegistryNode(RNT_DWORD, "tab size", new RegistryDataDword(8)));

	AVLTree *palette_analyser = new AVLTree(true);
	AVLTree *palette_generic = new AVLTree(true);
	AVLTree *palette_syntax = new AVLTree(true);
	AVLTree *palette_tags = new AVLTree(true);
	palette->insert(new RegistryNode(RNT_SUBDIR, "analyser", new RegistryDataSTree(palette_analyser)));
	AVLTree *analyser_default = new AVLTree(true);
	palette_analyser->insert(new RegistryNode(RNT_SUBDIR, "default", new RegistryDataSTree(analyser_default)));
	analyser_default->insert(new RegistryNode(256, "default", new palette_entry(0, 33288)));
	analyser_default->insert(new RegistryNode(256, "comment", new palette_entry(0, 1800)));
	analyser_default->insert(new RegistryNode(256, "label", new palette_entry(0, 34312)));
	analyser_default->insert(new RegistryNode(256, "number", new palette_entry(0, 776)));
	analyser_default->insert(new RegistryNode(256, "string", new palette_entry(0, 33800)));
	analyser_default->insert(new RegistryNode(256, "symbol-character", new palette_entry(0, 33544)));

	palette->insert(new RegistryNode(RNT_SUBDIR, "generic", new RegistryDataSTree(palette_generic)));
	AVLTree *palette_generic_black = new AVLTree(true);
	AVLTree *palette_generic_blue = new AVLTree(true);
	AVLTree *palette_generic_cyan = new AVLTree(true);
	AVLTree *palette_generic_gray = new AVLTree(true);
	AVLTree *palette_generic_gray2 = new AVLTree(true);
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "desktop", new RegistryDataString("black")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "dialog", new RegistryDataString("gray")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "help", new RegistryDataString("cyan")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "keyline", new RegistryDataString("black")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "menu", new RegistryDataString("gray2")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "special", new RegistryDataString("cyan")));
	palette_generic->insert(new RegistryNode(RNT_SYMLINK, "window", new RegistryDataString("blue")));
	palette_generic->insert(new RegistryNode(RNT_SUBDIR, "black", new RegistryDataSTree(palette_generic_black)));
	palette_generic->insert(new RegistryNode(RNT_SUBDIR, "blue", new RegistryDataSTree(palette_generic_blue)));
	palette_generic->insert(new RegistryNode(RNT_SUBDIR, "cyan", new RegistryDataSTree(palette_generic_cyan)));
	palette_generic->insert(new RegistryNode(RNT_SUBDIR, "gray", new RegistryDataSTree(palette_generic_gray)));
	palette_generic->insert(new RegistryNode(RNT_SUBDIR, "gray2", new RegistryDataSTree(palette_generic_gray2)));
	// black
	palette_generic_black->insert(new RegistryNode(256, "body", new palette_entry(0, 1792)));
	palette_generic_black->insert(new RegistryNode(256, "text focused", new palette_entry(0, 3)));
	palette_generic_black->insert(new RegistryNode(256, "text unfocused", new palette_entry(0, 8)));
	palette_generic_black->insert(new RegistryNode(256, "text shortcut", new palette_entry(0, 1792)));
	palette_generic_black->insert(new RegistryNode(256, "text shortcut selected", new palette_entry(0, 34312)));
	palette_generic_black->insert(new RegistryNode(256, "text selected", new palette_entry(0, 0)));
	palette_generic_black->insert(new RegistryNode(256, "text disabled", new palette_entry(0, 1795)));
	palette_generic_black->insert(new RegistryNode(256, "frame focused", new palette_entry(0, 1800)));
	palette_generic_black->insert(new RegistryNode(256, "frame unfocused", new palette_entry(0, 1800)));
	palette_generic_black->insert(new RegistryNode(256, "frame move-resize", new palette_entry(0, 33288)));
	palette_generic_black->insert(new RegistryNode(256, "frame killer", new palette_entry(0, 520)));
	palette_generic_black->insert(new RegistryNode(256, "scrollbar", new palette_entry(0, 7)));
	palette_generic_black->insert(new RegistryNode(256, "input focused", new palette_entry(0, 34561)));
	palette_generic_black->insert(new RegistryNode(256, "input unfocused", new palette_entry(0, 34561)));
	palette_generic_black->insert(new RegistryNode(256, "input selected", new palette_entry(0, 34562)));
	palette_generic_black->insert(new RegistryNode(256, "input clip-chars", new palette_entry(0, 33281)));
	palette_generic_black->insert(new RegistryNode(256, "button focused", new palette_entry(0, 34562)));
	palette_generic_black->insert(new RegistryNode(256, "button unfocused", new palette_entry(0, 2)));
	palette_generic_black->insert(new RegistryNode(256, "button shadow", new palette_entry(0, 8)));
	palette_generic_black->insert(new RegistryNode(256, "button shortcut", new palette_entry(0, 0)));
	palette_generic_black->insert(new RegistryNode(256, "list focused & selected", new palette_entry(0, 34561)));
	palette_generic_black->insert(new RegistryNode(256, "list focused & unselected", new palette_entry(0, 1792)));
	palette_generic_black->insert(new RegistryNode(256, "list unfocused & selected", new palette_entry(0, 34560)));
	palette_generic_black->insert(new RegistryNode(256, "list unfocused & unselected", new palette_entry(0, 0)));
	palette_generic_black->insert(new RegistryNode(256, "cluster focused", new palette_entry(0, 34563)));
	palette_generic_black->insert(new RegistryNode(256, "cluster unfocused", new palette_entry(0, 3)));
	palette_generic_black->insert(new RegistryNode(256, "cluster shortcut", new palette_entry(0, 34307)));
	// blue
	palette_generic_blue->insert(new RegistryNode(256, "body", new palette_entry(0, 769)));
	palette_generic_blue->insert(new RegistryNode(256, "text focused", new palette_entry(0, 34305)));
	palette_generic_blue->insert(new RegistryNode(256, "text unfocused", new palette_entry(0, 34561)));
	palette_generic_blue->insert(new RegistryNode(256, "text shortcut", new palette_entry(0, 1)));
	palette_generic_blue->insert(new RegistryNode(256, "text shortcut selected", new palette_entry(0, 1)));
	palette_generic_blue->insert(new RegistryNode(256, "text selected", new palette_entry(0, 257)));
	palette_generic_blue->insert(new RegistryNode(256, "text disabled", new palette_entry(0, 34305)));
	palette_generic_blue->insert(new RegistryNode(256, "frame focused", new palette_entry(0, 34568)));
	palette_generic_blue->insert(new RegistryNode(256, "frame unfocused", new palette_entry(0, 1800)));
	palette_generic_blue->insert(new RegistryNode(256, "frame move-resize", new palette_entry(0, 33288)));
	palette_generic_blue->insert(new RegistryNode(256, "frame killer", new palette_entry(0, 33288)));
	palette_generic_blue->insert(new RegistryNode(256, "scrollbar", new palette_entry(0, 259)));
	palette_generic_blue->insert(new RegistryNode(256, "input focused", new palette_entry(0, 34561)));
	palette_generic_blue->insert(new RegistryNode(256, "input unfocused", new palette_entry(0, 34561)));
	palette_generic_blue->insert(new RegistryNode(256, "input selected", new palette_entry(0, 33031)));
	palette_generic_blue->insert(new RegistryNode(256, "input clip-chars", new palette_entry(0, 33281)));
	palette_generic_blue->insert(new RegistryNode(256, "button focused", new palette_entry(0, 34562)));
	palette_generic_blue->insert(new RegistryNode(256, "button unfocused", new palette_entry(0, 2)));
	palette_generic_blue->insert(new RegistryNode(256, "button shadow", new palette_entry(0, 8)));
	palette_generic_blue->insert(new RegistryNode(256, "button shortcut", new palette_entry(0, 34312)));
	palette_generic_blue->insert(new RegistryNode(256, "list focused & selected", new palette_entry(0, 34563)));
	palette_generic_blue->insert(new RegistryNode(256, "list focused & unselected", new palette_entry(0, 769)));
	palette_generic_blue->insert(new RegistryNode(256, "list unfocused & selected", new palette_entry(0, 3)));
	palette_generic_blue->insert(new RegistryNode(256, "list unfocused & unselected", new palette_entry(0, 769)));
	palette_generic_blue->insert(new RegistryNode(256, "cluster focused", new palette_entry(0, 34563)));
	palette_generic_blue->insert(new RegistryNode(256, "cluster unfocused", new palette_entry(0, 0)));
	palette_generic_blue->insert(new RegistryNode(256, "cluster shortcut", new palette_entry(0, 0)));
	// cyan
	palette_generic_cyan->insert(new RegistryNode(256, "body", new palette_entry(0, 3)));
	palette_generic_cyan->insert(new RegistryNode(256, "text focused", new palette_entry(0, 8)));
	palette_generic_cyan->insert(new RegistryNode(256, "text unfocused", new palette_entry(0, 8)));
	palette_generic_cyan->insert(new RegistryNode(256, "text shortcut", new palette_entry(0, 34312)));
	palette_generic_cyan->insert(new RegistryNode(256, "text shortcut selected", new palette_entry(0, 34305)));
	palette_generic_cyan->insert(new RegistryNode(256, "text selected", new palette_entry(0, 8)));
	palette_generic_cyan->insert(new RegistryNode(256, "text disabled", new palette_entry(0, 1792)));
	palette_generic_cyan->insert(new RegistryNode(256, "frame focused", new palette_entry(0, 34568)));
	palette_generic_cyan->insert(new RegistryNode(256, "frame unfocused", new palette_entry(0, 8)));
	palette_generic_cyan->insert(new RegistryNode(256, "frame move-resize", new palette_entry(0, 33288)));
	palette_generic_cyan->insert(new RegistryNode(256, "frame killer", new palette_entry(0, 33288)));
	palette_generic_cyan->insert(new RegistryNode(256, "scrollbar", new palette_entry(0, 769)));
	palette_generic_cyan->insert(new RegistryNode(256, "input focused", new palette_entry(0, 34561)));
	palette_generic_cyan->insert(new RegistryNode(256, "input unfocused", new palette_entry(0, 34561)));
	palette_generic_cyan->insert(new RegistryNode(256, "input selected", new palette_entry(0, 34562)));
	palette_generic_cyan->insert(new RegistryNode(256, "input clip-chars", new palette_entry(0, 33281)));
	palette_generic_cyan->insert(new RegistryNode(256, "button focused", new palette_entry(0, 34562)));
	palette_generic_cyan->insert(new RegistryNode(256, "button unfocused", new palette_entry(0, 2)));
	palette_generic_cyan->insert(new RegistryNode(256, "button shadow", new palette_entry(0, 8)));
	palette_generic_cyan->insert(new RegistryNode(256, "button shortcut", new palette_entry(0, 34312)));
	palette_generic_cyan->insert(new RegistryNode(256, "list focused & selected", new palette_entry(0, 34561)));
	palette_generic_cyan->insert(new RegistryNode(256, "list focused & unselected", new palette_entry(0, 3)));
	palette_generic_cyan->insert(new RegistryNode(256, "list unfocused & selected", new palette_entry(0, 34563)));
	palette_generic_cyan->insert(new RegistryNode(256, "list unfocused & unselected", new palette_entry(0, 3)));
	palette_generic_cyan->insert(new RegistryNode(256, "cluster focused", new palette_entry(0, 34563)));
	palette_generic_cyan->insert(new RegistryNode(256, "cluster unfocused", new palette_entry(0, 3)));
	palette_generic_cyan->insert(new RegistryNode(256, "cluster shortcut", new palette_entry(0, 34307)));
	// gray
	palette_generic_gray->insert(new RegistryNode(256, "body", new palette_entry(0, 7)));
	palette_generic_gray->insert(new RegistryNode(256, "text focused", new palette_entry(0, 34568)));
	palette_generic_gray->insert(new RegistryNode(256, "text unfocused", new palette_entry(0, 8)));
	palette_generic_gray->insert(new RegistryNode(256, "text shortcut", new palette_entry(0, 34312)));
	palette_generic_gray->insert(new RegistryNode(256, "text shortcut selected", new palette_entry(0, 34312)));
	palette_generic_gray->insert(new RegistryNode(256, "text selected", new palette_entry(0, 34561)));
	palette_generic_gray->insert(new RegistryNode(256, "text disabled", new palette_entry(0, 32776)));
	palette_generic_gray->insert(new RegistryNode(256, "frame focused", new palette_entry(0, 34568)));
	palette_generic_gray->insert(new RegistryNode(256, "frame unfocused", new palette_entry(0, 8)));
	palette_generic_gray->insert(new RegistryNode(256, "frame move-resize", new palette_entry(0, 33288)));
	palette_generic_gray->insert(new RegistryNode(256, "frame killer", new palette_entry(0, 33288)));
	palette_generic_gray->insert(new RegistryNode(256, "scrollbar", new palette_entry(0, 769)));
	palette_generic_gray->insert(new RegistryNode(256, "input focused", new palette_entry(0, 34561)));
	palette_generic_gray->insert(new RegistryNode(256, "input unfocused", new palette_entry(0, 34561)));
	palette_generic_gray->insert(new RegistryNode(256, "input selected", new palette_entry(0, 34562)));
	palette_generic_gray->insert(new RegistryNode(256, "input clip-chars", new palette_entry(0, 33281)));
	palette_generic_gray->insert(new RegistryNode(256, "button focused", new palette_entry(0, 34562)));
	palette_generic_gray->insert(new RegistryNode(256, "button unfocused", new palette_entry(0, 2)));
	palette_generic_gray->insert(new RegistryNode(256, "button shadow", new palette_entry(0, 8)));
	palette_generic_gray->insert(new RegistryNode(256, "button shortcut", new palette_entry(0, 34312)));
	palette_generic_gray->insert(new RegistryNode(256, "list focused & selected", new palette_entry(0, 34562)));
	palette_generic_gray->insert(new RegistryNode(256, "list focused & unselected", new palette_entry(0, 3)));
	palette_generic_gray->insert(new RegistryNode(256, "list unfocused & selected", new palette_entry(0, 34563)));
	palette_generic_gray->insert(new RegistryNode(256, "list unfocused & unselected", new palette_entry(0, 3)));
	palette_generic_gray->insert(new RegistryNode(256, "cluster focused", new palette_entry(0, 34563)));
	palette_generic_gray->insert(new RegistryNode(256, "cluster unfocused", new palette_entry(0, 3)));
	palette_generic_gray->insert(new RegistryNode(256, "cluster shortcut", new palette_entry(0, 34307)));
	// gray2
	palette_generic_gray2->insert(new RegistryNode(256, "body", new palette_entry(0, 7)));
	palette_generic_gray2->insert(new RegistryNode(256, "text focused", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "text unfocused", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "text shortcut", new palette_entry(0, 1032)));
	palette_generic_gray2->insert(new RegistryNode(256, "text shortcut selected", new palette_entry(0, 1032)));
	palette_generic_gray2->insert(new RegistryNode(256, "text selected", new palette_entry(0, 2)));
	palette_generic_gray2->insert(new RegistryNode(256, "text disabled", new palette_entry(0, 1792)));
	palette_generic_gray2->insert(new RegistryNode(256, "frame focused", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "frame unfocused", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "frame move-resize", new palette_entry(0, 33288)));
	palette_generic_gray2->insert(new RegistryNode(256, "frame killer", new palette_entry(0, 33288)));
	palette_generic_gray2->insert(new RegistryNode(256, "scrollbar", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "input focused", new palette_entry(0, 34561)));
	palette_generic_gray2->insert(new RegistryNode(256, "input unfocused", new palette_entry(0, 34561)));
	palette_generic_gray2->insert(new RegistryNode(256, "input selected", new palette_entry(0, 34562)));
	palette_generic_gray2->insert(new RegistryNode(256, "input clip-chars", new palette_entry(0, 33281)));
	palette_generic_gray2->insert(new RegistryNode(256, "button focused", new palette_entry(0, 34562)));
	palette_generic_gray2->insert(new RegistryNode(256, "button unfocused", new palette_entry(0, 2)));
	palette_generic_gray2->insert(new RegistryNode(256, "button shadow", new palette_entry(0, 8)));
	palette_generic_gray2->insert(new RegistryNode(256, "button shortcut", new palette_entry(0, 34312)));
	palette_generic_gray2->insert(new RegistryNode(256, "list focused & selected", new palette_entry(0, 34562)));
	palette_generic_gray2->insert(new RegistryNode(256, "list focused & unselected", new palette_entry(0, 3)));
	palette_generic_gray2->insert(new RegistryNode(256, "list unfocused & selected", new palette_entry(0, 34563)));
	palette_generic_gray2->insert(new RegistryNode(256, "list unfocused & unselected", new palette_entry(0, 3)));
	palette_generic_gray2->insert(new RegistryNode(256, "cluster focused", new palette_entry(0, 34563)));
	palette_generic_gray2->insert(new RegistryNode(256, "cluster unfocused", new palette_entry(0, 3)));
	palette_generic_gray2->insert(new RegistryNode(256, "cluster shortcut", new palette_entry(0, 0)));
	palette->insert(new RegistryNode(RNT_SUBDIR, "syntax", new RegistryDataSTree(palette_syntax)));
	AVLTree *c = new AVLTree(true);
	palette_syntax->insert(new RegistryNode(RNT_SUBDIR, "c", new RegistryDataSTree(c)));
	AVLTree *default1 = new AVLTree(true);
	c->insert(new RegistryNode(RNT_SUBDIR, "default", new RegistryDataSTree(default1)));
	default1->insert(new RegistryNode(256, "whitespace", new palette_entry(0, 34312)));
	default1->insert(new RegistryNode(256, "comment", new palette_entry(0, 1800)));
	default1->insert(new RegistryNode(256, "identifier", new palette_entry(0, 34312)));
	default1->insert(new RegistryNode(256, "reserved", new palette_entry(0, 34568)));
	default1->insert(new RegistryNode(256, "integer number", new palette_entry(0, 34056)));
	default1->insert(new RegistryNode(256, "float number", new palette_entry(0, 33032)));
	default1->insert(new RegistryNode(256, "string", new palette_entry(0, 33544)));
	default1->insert(new RegistryNode(256, "character", new palette_entry(0, 33544)));
	default1->insert(new RegistryNode(256, "symbol", new palette_entry(0, 34568)));
	default1->insert(new RegistryNode(256, "preprocess", new palette_entry(0, 33288)));
	default1->insert(new RegistryNode(256, "meta", new palette_entry(0, 33032)));
	palette->insert(new RegistryNode(RNT_SUBDIR, "tags", new RegistryDataSTree(palette_tags)));
	AVLTree *tag_default = new AVLTree(true);
	palette_tags->insert(new RegistryNode(RNT_SUBDIR, "default", new RegistryDataSTree(tag_default)));
	tag_default->insert(new RegistryNode(256, "edit-tag cursor select", new palette_entry(0, 34564)));
	tag_default->insert(new RegistryNode(256, "edit-tag cursor edit", new palette_entry(0, 34564)));
	tag_default->insert(new RegistryNode(256, "edit-tag cursor unfocused", new palette_entry(0, 34568)));
	tag_default->insert(new RegistryNode(256, "edit-tag selected", new palette_entry(0, 32775)));
	tag_default->insert(new RegistryNode(256, "edit-tag modified", new palette_entry(0, 33800)));
	tag_default->insert(new RegistryNode(256, "edit-tag", new palette_entry(0, 1800)));
	tag_default->insert(new RegistryNode(256, "sel-tag cursor focused", new palette_entry(0, 34563)));
	tag_default->insert(new RegistryNode(256, "sel-tag cursor unfocused", new palette_entry(0, 3)));
	tag_default->insert(new RegistryNode(256, "sel-tag", new palette_entry(0, 34568)));

	// build node_types tree
	node_types = new AVLTree(true);

	struct bla {
		const char *identifier;
		ht_registry_node_type type;
		create_empty_registry_data_func create_empty_registry_data;
	};
	bla b[]=
	{
		{"subdir", RNT_SUBDIR, NULL},
		{"symlink", RNT_SYMLINK, create_empty_symlink},
		{"dword", RNT_DWORD, create_empty_dword},
		{"string", RNT_STRING, create_empty_string},
		{"raw", RNT_RAW, create_empty_raw},
	};

	for (uint i=0; i<sizeof (b) / sizeof b[0]; i++) {
		ht_registry_node_type_desc *d=new ht_registry_node_type_desc(
			b[i].type, b[i].identifier, b[i].create_empty_registry_data);
		node_types->insert(d);
	}
		ht_registry_node_type_desc *d=new ht_registry_node_type_desc(
			256, "palette", create_empty_palette_entry);
		node_types->insert(d);
#endif
}

void Registry::done()
{
	delete node_types;
	delete root;
}

int Registry::create_node(const char *key, ht_registry_node_type type)
{
	const char *name;
	RegistryNode *m;
	if (!splitfind(key, &name, &m)) return EINVAL;

	if (!valid_nodename(name)) return EINVAL;

	ht_registry_node_type_desc *d = get_node_type_desc(type, NULL);
	if (d && d->create_empty_registry_data) {
		RegistryNode t(0, name, NULL);
		if (((RegistryDataSTree*)m->data)->tree->find(&t) != invObjHandle) return EEXIST;

		RegistryData *data = d->create_empty_registry_data();
		RegistryNode *n=new RegistryNode(type, name, data);
		((RegistryDataSTree*)m->data)->tree->insert(n);
		return 0;
	}

	return ENOSYS;
}

int Registry::create_subdir(const char *key)
{
	const char *name;
	RegistryNode *m;
	if (!splitfind(key, &name, &m)) return EINVAL;

	if (!valid_nodename(name)) return EINVAL;

	AVLTree *s=new AVLTree(true);

	RegistryNode *n=new RegistryNode(RNT_SUBDIR, name, new RegistryDataSTree(s));
	bool ins;
	((RegistryDataSTree*)m->data)->tree->findOrInsert(n, ins);
	if (!ins) {
		delete n;
		return EEXIST;
	}
	return 0;
}

int Registry::delete_node(const char *key)
{
	Container *dir;
	RegistryNode *n = find_entry_i(&dir, key, false);
	if (!n) return ENOENT;
	const char *s = strrchr(key, '/');
	if (s) s++; else s=key;
	RegistryNode ss(0, s, NULL);
	return dir->delObj(&ss) ? 0 : ENOENT;
}

void Registry::debug_dump()
{
#if 0
//	FILE *f=fopen("", "");
	debug_dump_i(stderr, ((RegistryDataSTree*)root->data)->tree, 0);
//	fclose(f);
#endif
}

void Registry::debug_dump_i(FILE *f, Container *t, int ident)
{
#if 0
	ht_data_string *key=NULL;
	RegistryNode *n;
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
			debug_dump_i(f, ((RegistryDataSTree*)n->data)->tree, ident+1);
			for (int i=0; i<ident; i++) fprintf(f, "     ");
			fprintf(f, "}\n");
			break;
		case RNT_RAW:
			fprintf(f, "= (raw) nyi!\n");
			break;
		default: {
			char *name = lookup_node_type_name(n->type);
			if (!name) name = "?";
			fprintf(f, "= ('%s'=%d)\n", name, n->type);
			break;
		}
		}
	}
#endif
}

RegistryNode *Registry::enum_next(const char *dir, RegistryNode *prevkey)
{
	rec_depth = 0;
	RegistryNode *n = find_entry_i(NULL, dir, true);
	if (n) {
		if (n->type != RNT_SUBDIR) return NULL;
		Container *t = ((RegistryDataSTree*)n->data)->tree;

		if (prevkey) {
			return (RegistryNode *)t->get(t->findG(prevkey));
		} else {
			return (RegistryNode *)t->get(t->findFirst());
		}
	}
	return NULL;
}

RegistryNode *Registry::enum_prev(const char *dir, RegistryNode *prevkey)
{
	rec_depth = 0;
	RegistryNode *n = find_entry_i(NULL, dir, true);
	if (n) {
		if (n->type != RNT_SUBDIR) return NULL;
		Container *t = ((RegistryDataSTree*)n->data)->tree;

		if (prevkey) {
			return (RegistryNode *)t->get(t->findL(prevkey));
		} else {
			return (RegistryNode *)t->get(t->findLast());
		}
	}
	return NULL;
}

bool Registry::find_any_entry(const char *key, RegistryNode **node)
{
	rec_depth = 0;
	RegistryNode *n = find_entry_i(NULL, key, true);
	if (n) {
		*node = n;
		return true;
	}
	return false;
}

bool Registry::find_data_entry(const char *key, RegistryNode **node, bool follow_symlinks, Container **rdir)
{
	RegistryNode *n = find_entry_i(rdir, key, follow_symlinks);
	if (n) {
		if (n->type == RNT_SUBDIR) return false;
		*node = n;
		return true;
	}
	return false;
}

RegistryNode *Registry::find_entry_i(Container **rdir, const char *key, bool follow_symlinks)
{
	RegistryNode *dir = root;
	const char *s;
	if (key[0]=='/') key++;
	while (1) {
		s = strchr(key, '/');
		if (s) {
			String t((const byte *)key, s - key);
			dir = find_entry_get_subdir(((RegistryDataSTree*)dir->data)->tree, t.contentChar());
			if (!dir) break;
			key = s+1;
		} else {
			RegistryNode *n;
			if (*key==0) {
				n = dir;
			} else {
				n = find_entry_get_data(((RegistryDataSTree*)dir->data)->tree, key, follow_symlinks);
			}
			if (rdir) *rdir=((RegistryDataSTree*)dir->data)->tree;
			return n;
		}
	}
	return NULL;
}

RegistryNode *Registry::find_entry_get_node(Container *dir, const char *nodename)
{
	if (nodename) {
		RegistryNode t(0, nodename, NULL);
		RegistryNode *n=(RegistryNode*)dir->get(dir->find(&t));
		return n;
	}
	return NULL;
}

RegistryNode *Registry::find_entry_get_subdir(Container *dir, const char *nodename)
{
	RegistryNode *n=find_entry_get_node(dir, nodename);
start:
	if (!n) return 0;
	switch (n->type) {
		case RNT_SYMLINK: {
			rec_depth++;
			if (rec_depth > MAX_SYMLINK_REC_DEPTH) return 0;
			char *sl=((RegistryDataString*)n->data)->value;
			if (sl[0] == '/') {
				n = find_entry_i(NULL, sl, true);
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

RegistryNode *Registry::find_entry_get_data(Container *dir, const char *nodename, bool follow_symlinks)
{
	RegistryNode *n = find_entry_get_node(dir, nodename);
start:
	if (!n) return NULL;
	if (follow_symlinks && n->type == RNT_SYMLINK) {
		rec_depth++;
		if (rec_depth > MAX_SYMLINK_REC_DEPTH) return NULL;
		char *sl = ((RegistryDataString*)n->data)->value;
		if (sl[0] == '/') {
			n = find_entry_i(NULL, sl, true);
			goto start;
		} else {
			return find_entry_get_data(dir, sl, follow_symlinks);
		}
	}
	return n;
}

ht_registry_node_type Registry::have_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data)
{
	ht_registry_node_type t=lookup_node_type(identifier);
	if (!t) t=register_node_type(identifier, create_empty_registry_data);
	return t;
}

void Registry::load(ObjectStream &f)
{
	GET_OBJECT(f, node_types);
	GET_OBJECT(f, root);
}

ht_registry_node_type_desc *Registry::get_node_type_desc(ht_registry_node_type t, const char **identifier)
{
	ht_registry_node_type_desc *data = NULL;
	firstThat(ht_registry_node_type_desc, data, *node_types, t == data->type);
	if (data && identifier) *identifier = data->name;
	return data;
}

ht_registry_node_type Registry::lookup_node_type(const char *identifier)
{
	ht_registry_node_type_desc s(0, identifier, NULL);
	ht_registry_node_type_desc *d=(ht_registry_node_type_desc*)node_types->get(node_types->find(&s));
	return d ? d->type : 0;
}

ObjectID Registry::getObjectID() const
{
	return ATOM_HT_REGISTRY;
}

ht_registry_node_type Registry::register_node_type(const char *identifier, create_empty_registry_data_func create_empty_registry_data)
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

	if (node_types->find(v) != invObjHandle) {
		delete v;
		return RNT_INVALID;
	} else {
		node_types->insert(v);
		return t;
	}
}

int Registry::set_dword(const char *key, uint32 d)
{
	return set_node(key, RNT_DWORD, new RegistryDataDword(d));
}

int Registry::set_raw(const char *key, const void *data, uint size)
{
	return set_node(key, RNT_RAW, new RegistryDataRaw(data, size));
}

int Registry::set_node(const char *key, ht_registry_node_type type, RegistryData *data)
{
	RegistryNode *n = find_entry_i(NULL, key, false);
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

int Registry::set_string(const char *key, const char *string)
{
	return set_node(key, RNT_STRING, new RegistryDataString(string));
}

int Registry::set_symlink(const char *key, const char *dest)
{
	return set_node(key, RNT_SYMLINK, new RegistryDataString(dest));
}

bool Registry::splitfind(const char *key, const char **name, RegistryNode **node)
{
	char dir[256]; /* FIXME: possible buffer overflow */
	const char *n = strrchr(key, '/');
	if (n) {
		ht_strlcpy(dir, key, n-key+1);
		n++;
	} else {
		dir[0]=0;
		n=key;
	}

	RegistryNode *m = find_entry_i(NULL, dir, true);
	if (!m) return 0;
	if (m->type != RNT_SUBDIR) return 0;
	*node = m;
	*name = n;
	return 1;
}

void Registry::store(ObjectStream &f) const
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

static bool valid_char(unsigned char *bitmap, char c)
{
	int o=c/8;
	int p=c%8;
	return BITBIT(bitmap[o], p);
}

bool Registry::valid_nodename(const char *nodename)
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

uint32 get_config_dword(const char *ident, uint32 def)
{
	String e;
	e = "/config/";
	e.append(ident);
	RegistryNode *n;
	Container *dir;
	if (registry->find_data_entry(e.contentChar(), &n, true, &dir)) {
		if (n->type == RNT_DWORD) {
			RegistryDataDword *s = (RegistryDataDword *)n->data;
			return s->value;
		} else {
			const char *q = "?";
			registry->get_node_type_desc(n->type, &q);
			LOG_EX(LOG_ERROR, "registry key '%y' not of type %s, but: %s", &e, "dword", q);
		}
	} else {
		String f(ident);
		String dirname, filename;
		f.rightSplit('/', dirname, filename);
		dir->insert(new RegistryNode(RNT_DWORD, filename.contentChar(), new RegistryDataDword(def)));
		LOG_EX(LOG_ERROR, "registry key '%y' not found", &e);
	}
	return def;
}

char *get_config_string(const char *ident)
{
	String e;
	e = "/config/";
	e.append(ident);
	RegistryNode *n;
	if (registry->find_data_entry(e.contentChar(), &n, true)) {
		if (n->type == RNT_STRING) {
			RegistryDataString *s = (RegistryDataString *)n->data;
			return ht_strdup(s->value);
		} else {
			const char *q = "?";
			registry->get_node_type_desc(n->type, &q);
			LOG_EX(LOG_ERROR, "registry key '%y' not of type %s, but: %s", &e, "string", q);
		}
	} else LOG_EX(LOG_ERROR, "registry key '%y' not found", &e);
	return NULL;
}

BUILDER(ATOM_HT_REGISTRY, Registry, Object);
BUILDER(ATOM_HT_REGISTRY_NODE, RegistryNode, Object);
BUILDER(ATOM_HT_REGISTRY_DATA_STREE, RegistryDataSTree, RegistryData);
BUILDER(ATOM_HT_REGISTRY_DATA_DWORD, RegistryDataDword, RegistryData);
BUILDER(ATOM_HT_REGISTRY_DATA_RAW, RegistryDataRaw, RegistryData);
BUILDER(ATOM_HT_REGISTRY_DATA_STRING, RegistryDataString, RegistryData);
BUILDER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc, Object);

/*
 *	INIT
 */

#include "cstream.h"
bool init_registry()
{
	REGISTER(ATOM_HT_REGISTRY, Registry);
	REGISTER(ATOM_HT_REGISTRY_NODE, RegistryNode);
	REGISTER(ATOM_HT_REGISTRY_DATA_STREE, RegistryDataSTree);
	REGISTER(ATOM_HT_REGISTRY_DATA_DWORD, RegistryDataDword);
	REGISTER(ATOM_HT_REGISTRY_DATA_RAW, RegistryDataRaw);
	REGISTER(ATOM_HT_REGISTRY_DATA_STRING, RegistryDataString);
	REGISTER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc);
//	registerAtom(ATOM_HT_CREATE_EMPTY_SUBDIR, (void*));
	registerAtom(ATOM_HT_CREATE_EMPTY_SYMLINK, (void*)create_empty_symlink);
	registerAtom(ATOM_HT_CREATE_EMPTY_DWORD, (void*)create_empty_dword);
	registerAtom(ATOM_HT_CREATE_EMPTY_STRING, (void*)create_empty_string);
	registerAtom(ATOM_HT_CREATE_EMPTY_RAW, (void*)create_empty_raw);

	/*
	 *	load default registry
	 */
#if ! GENERATE_NEW_REGISTRY
	ConstMemMapFile f(default_reg, sizeof default_reg);
	CompressedStream c(&f, false);
	ObjectStreamBin o(&c, false);

	GET_OBJECT(o, registry);
#else
	registry = new Registry;
	registry->init();

	LocalFile f("ht.reg", IOAM_WRITE, FOM_CREATE);
	CompressedStream c(&f, false);
	ObjectStreamBin o(&c, false);

	PUT_OBJECT(o, registry);
#endif

	return true;
}

/*
 *	DONE
 */

void done_registry()
{
	UNREGISTER(ATOM_HT_REGISTRY, Registry);
	UNREGISTER(ATOM_HT_REGISTRY_NODE, RegistryNode);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_STREE, RegistryDataSTree);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_DWORD, RegistryDataDword);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_RAW, RegistryDataRaw);
	UNREGISTER(ATOM_HT_REGISTRY_DATA_STRING, RegistryDataString);
	UNREGISTER(ATOM_HT_REGISTRY_NODE_TYPE_DESC, ht_registry_node_type_desc);
//	unregisterAtom(ATOM_HT_CREATE_EMPTY_SUBDIR);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_SYMLINK);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_DWORD);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_STRING);
	unregisterAtom(ATOM_HT_CREATE_EMPTY_RAW);

	registry->done();
	delete registry;
}

