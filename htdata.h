/* 
 *	HT Editor
 *	htdata.h
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

#ifndef __HTDATA_H__
#define __HTDATA_H__

#include "common.h"

#define ATOM_HT_DATA_UINT		MAGICD("DAT\x00")
#define ATOM_HT_DATA_UINT32		MAGICD("DAT\x01")
#define ATOM_HT_DATA_MEM		MAGICD("DAT\x02")

#define ATOM_HT_STREE			MAGICD("DAT\x10")
#define ATOM_HT_CLIST			MAGICD("DAT\x11")

#define ATOM_COMPARE_KEYS_HT_DATA	MAGICD("DAT\x20")
#define ATOM_COMPARE_KEYS_INT		MAGICD("DAT\x21")
#define ATOM_COMPARE_KEYS_UINT		MAGICD("DAT\x22")

typedef int (*compare_keys_func_ptr)(Object *key_a, Object *key_b);

/*
 *	ht_data_uint
 */
class ht_data_uint: public Object {
public:
	uint value;

	ht_data_uint(uint v = 0);
	/* overwritten */
	virtual	int load(ht_object_stream *s);
	virtual	void store(ht_object_stream *s);
	virtual	OBJECT_ID object_id() const;
};

/*
 *	ht_data_uint32
 */
class ht_data_uint32: public Object {
public:
	uint32 value;

		ht_data_uint32(uint32 v = 0);
	/* overwritten */
	virtual	int load(ht_object_stream *s);
	virtual	void store(ht_object_stream *s);
	virtual	OBJECT_ID object_id() const;
};

/*
 *	ht_data_ptr
 */
class ht_data_ptr: public Object {
public:
	const void *value;

	ht_data_ptr(const void *v=0);
};

/*
 *	ht_data_mem
 */
class ht_data_mem: public Object {
public:
	void *value;
	uint size;

		ht_data_mem(const void *v = 0, uint size = 0);
	virtual	~ht_data_mem();
	/* overwritten */
	virtual	int load(ht_object_stream *s);
	virtual	void store(ht_object_stream *s);
	virtual	OBJECT_ID object_id() const;
};

/*
 *	ht_tree
 */
struct ht_tree_node {
	Object *key;
	Object *value;
	ht_tree_node *left, *right;
};

class ht_tree: public Object {
public:
	compare_keys_func_ptr compare_keys;

		void init(compare_keys_func_ptr compare_keys);
	virtual	void done();
	virtual	void destroy();
	/* new */
	virtual	void balance();
	virtual	uint count();
	virtual	bool del(Object *key);
	virtual Object *enum_next(Object **value, Object *prevkey);
	virtual Object *enum_prev(Object **value, Object *nextkey);
	virtual	Object *get(Object *key);
	virtual	Object *get_insert(Object *key);
	virtual	bool insert(Object *key, Object *value);
	virtual void set_compare_keys(compare_keys_func_ptr new_compare_keys);
};

/*
 *	ht_stree	(simple tree)
 */
class ht_stree: public ht_tree {
public:
	ht_tree_node *root;
	uint node_count;

		void enum_next_i(ht_tree_node *node, Object *prevkey, ht_tree_node **retv);
		void enum_prev_i(ht_tree_node *node, Object *nextkey, ht_tree_node **retv);
		void free_all(ht_tree_node *node);
		void free_skeleton(ht_tree_node *node);
		ht_tree_node *get_leftmost_node(ht_tree_node *node);
		bool get_node_and_parent(Object *key, ht_tree_node **node, ht_tree_node **parent_node, int *direction);
		ht_tree_node *get_node_i(Object *key);
		ht_tree_node *get_rightmost_node(ht_tree_node *node);
		void insert_ltable(ht_tree_node **node, ht_tree_node **start, ht_tree_node **end);
	virtual	void populate_ltable(ht_tree_node ***ltable, ht_tree_node *node);
public:
		void init(compare_keys_func_ptr compare_keys);
	virtual	void done();
	virtual	void destroy();
	/* overwritten */
	virtual	void balance();
	virtual	uint count();
	virtual	bool del(Object *key);
	virtual	void empty();
	virtual Object *enum_next(Object **value, Object *prevkey);
	virtual Object *enum_prev(Object **value, Object *nextkey);
	virtual	Object *get(Object *key);
	virtual	bool insert(Object *key, Object *value);
	virtual	int load(ht_object_stream *s);
	virtual	OBJECT_ID object_id() const;
	virtual	void store(ht_object_stream *s);
	virtual void set_compare_keys(compare_keys_func_ptr new_compare_keys);
};

/*
 *	ht_dtree (dead node tree)
 */

#define DEFAULT_MAX_UB_DELETE 500
#define DEFAULT_MAX_UB_INSERT 500

class ht_dtree: public ht_stree {
protected:
	uint dead_node_count;
	uint ub_delete, max_ub_delete;
	uint ub_insert, max_ub_insert;

		void hardcount(uint *nc, uint *dnc);
	virtual	void populate_ltable(ht_tree_node ***ltable, ht_tree_node *node);
	virtual	void populate_ltable_free_dead_nodes(ht_tree_node ***ltable, ht_tree_node *node);
public:
		void init(compare_keys_func_ptr compare_keys, uint _max_ub_delete=DEFAULT_MAX_UB_DELETE, uint _max_ub_insert=DEFAULT_MAX_UB_INSERT);
	virtual	void done();
	/* overwritten */
	virtual	uint count();
	virtual	bool del(Object *key);
	virtual Object *enum_next(Object **value, Object *prevkey);
	virtual Object *enum_prev(Object **value, Object *nextkey);
	virtual	bool insert(Object *key, Object *value);
	virtual void set_compare_keys(compare_keys_func_ptr new_compare_keys);
};

/*
 *	ht_list
 */
#define LIST_UNDEFINED 0xffffffff

class ht_list: public Object {
protected:
	compare_keys_func_ptr compare_keys;
public:
		void init(compare_keys_func_ptr compare_keys=0);
	virtual	void done();
	virtual	void destroy();
	/* new */
	virtual	void append(Object *data);
	virtual	uint count();
		void copy_to(uint i, uint count, ht_list *destlist);
	virtual	ht_list *cut(uint i, uint count);
	virtual	bool del(uint i);
		bool del_multiple(uint i, uint count);
	virtual	void empty();
	virtual	uint find(Object *data);
	virtual	Object *get(uint i);
	virtual	void insert(Object *data);
	virtual	void insert_after(Object *data, uint i);
	virtual	void insert_before(Object *data, uint i);
	virtual	void move(uint source, uint dest);
	virtual	void move_multiple(uint source, uint dest, uint count);
	virtual	void prepend(Object *data);
	virtual	Object *remove(uint i);
		bool remove_multiple(uint i, uint count);
	virtual	bool set(uint i, Object *data);
	virtual	bool sort();
};

/*
 *	ht_clist
 */

class ht_clist: public ht_list {
protected:
	Object **items;
	uint c_size, c_entry_count;
	uint enum_pos;

		void extend_list();
		void do_free(uint i);
		void do_remove(uint i);
//	virtual	bool qsort_i(uint l, uint r);
public:
		void init(compare_keys_func_ptr compare_keys=0);
	virtual	void done();
	virtual	void destroy();
	/* overwritten */
	virtual	void append(Object *data);
	virtual	uint count();
	virtual	ht_list *cut(uint i, uint count);
	virtual	bool del(uint i);
	virtual	Object *duplicate();
	virtual void empty();
	virtual	uint find(Object *data);
	virtual	Object *get(uint i);
	virtual	void insert(Object *data);
	virtual	void insert_after(Object *data, uint i);
	virtual	void insert_before(Object *data, uint i);
	virtual	int  load(ht_object_stream *s);
	virtual	void move(uint source, uint dest);
	virtual	void move_multiple(uint source, uint dest, uint count);
	virtual	OBJECT_ID object_id() const;
	virtual	void prepend(Object *data);
	virtual	Object *remove(uint i);
	virtual	bool set(uint i, Object *data);
	virtual	bool sort();
	virtual	void store(ht_object_stream *s);
};

/*
 *	ht_sorted_list
 */
class ht_sorted_list: public ht_clist {
public:
		void init(compare_keys_func_ptr compare_keys);
	virtual	void done();
	/* overwritten */
	virtual	void append(Object *data);
	virtual	uint find(Object *data);
	virtual	void insert(Object *data);
	virtual	void insert_after(Object *data, uint i);
	virtual	void insert_before(Object *data, uint i);
	virtual	void move(uint source, uint dest);
	virtual	void move_multiple(uint source, uint dest, uint count);
	virtual	void prepend(Object *data);
	virtual	bool set(uint i, Object *data);
};

/*
 *	ht_stack
 */
class ht_stack: public ht_clist {
public:
	/* new */
		Object *pop();
		void	push(Object *data);
};

/*
 *	ht_queue
 */
class ht_queue: public ht_clist {
public:
	/* new */
		void	enqueue(Object *data);
		Object *dequeue();
	/* sepp-wrap */
		Object *pop();
		void	push(Object *data);
};

int compare_keys_ht_data(Object *key_a, Object *key_b);
int compare_keys_int(Object *key_a, Object *key_b);
int compare_keys_uint(Object *key_a, Object *key_b);

/*
 *	char_set
 */

#define CS_SETSIZE 256

typedef struct char_set {
  unsigned char char_bits [((CS_SETSIZE) + 7) / 8];
} char_set;

#define CS_SET(n, p)    ((p)->char_bits[(n) / 8] |= (1 << ((n) & 7)))
#define CS_CLR(n, p)	((p)->char_bits[(n) / 8] &= ~(1 << ((n) & 7)))
#define CS_ISSET(n, p)	((p)->char_bits[(n) / 8] & (1 << ((n) & 7)))
#define CS_ZERO(p)	memset ((void *)(p), 0, sizeof (*(p)))

/*
 *
 */

#define BITMAP(a0, a1, a2, a3, a4, a5, a6, a7) (((a0)<<0) | ((a1)<<1) | ((a2)<<2) | ((a3)<<3) | ((a4)<<4) | ((a5)<<5) | ((a6)<<6) | ((a7)<<7))

#define BITBIT(bitmap, p) ((bitmap)>>(p)&1)

/*
 *	simple int hash
 */

struct int_hash {
	int value;
	char *desc;
};

char *matchhash(int value, int_hash *hash_table);

/*
 *	INIT
 */

bool init_data();

/*
 *	DONE
 */

void done_data();

#endif /* __HTDATA_H__ */
