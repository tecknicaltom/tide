/*
 *	HT Editor
 *	htapp.h
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

#ifndef __HTAPP_H__
#define __HTAPP_H__

#include "htctrl.h"
#include "htdialog.h"
#include "htformat.h"

// file open modes

#define FOM_AUTO				0
#define FOM_BIN					1
#define FOM_TEXT				2

//
#define VIEWERGROUP_NAME			"viewergroup"

/*
 *	CLASS UiStatus
 */

#define STATUS_DEFAULT_FORMAT "%a %L %t %d"
#define STATUS_ESCAPE '%'
#define STATUS_ANALY_ACTIVE 'a'
#define STATUS_ANALY_LINES 'L'
#define STATUS_TIME 't'
#define STATUS_DATE 'd'
#define STATUS_WORKBUFLEN 80

/*
 *	CLASS UiStatus
 */

class UiStatus: public UiView {
protected:
	int		idle_count;
	char		*format;
	char		workbuf[STATUS_WORKBUFLEN];
	int		clear_len;
	int		analy_ani;
public:
		void init(Bounds *b);
	virtual	void done();
	virtual	void draw();
	virtual	void handlemsg(htmsg *msg);
	virtual	bool idle();
	virtual void getminbounds(int *width, int *height);
private:
		void render();
	virtual	const char *defaultpalette();
};

/*
 *	CLASS UiKeyline
 */

class UiKeyline: public UiView {
public:
		void init(Bounds *b);
	virtual	void done();
	/* overwritten */
	virtual	void draw();
	virtual	const char *defaultpalette();
	virtual void getminbounds(int *width, int *height);
};

/*
 *	CLASS UiDesktop
 */

class UiDesktop: public UiView {
public:
		void init(Bounds *b);
	/* overwritten */
	virtual	void draw();
	virtual	const char *defaultpalette();
};

/*
 *	CLASS ht_logviewer
 */

class ht_log_msg: public Object {
public:
	vcp color;
	char *msg;
	ht_log_msg(vcp Color, char *Msg);
	~ht_log_msg();
};

typedef unsigned int LogColor;

class ht_log: public Array {
protected:
	uint maxlinecount;

	void deletefirstline();
	void insertline(LogColor c, char *line);
public:
	ht_log();
/* new */
	void log(LogColor c, char *line);
};

class ht_logviewer: public UiViewer {
private:
	ht_log *lines;
	bool own_lines;
	int ofs, xofs;
	UiWindow *window;

	/* new */
	int cursor_up(int n);
	int cursor_down(int n);
	bool get_vscrollbar_pos(int *pstart, int *psize);
	void update();
public:
		void init(Bounds *b, UiWindow *window, ht_log *log, bool own_log);
	virtual	void done();
	/* overwritten */
	virtual	void draw();
	virtual	void handlemsg(htmsg *msg);
};

/*
 *	CLASS ht_vstate_history_entry
 */

class ht_vstate_history_entry: public Object {
public:
	Object *data;
	UiView *view;

	ht_vstate_history_entry(Object *data, UiView *view);
	~ht_vstate_history_entry();
};

/*
 *	CLASS UiFileWindow
 */

class UiFileWindow: public UiWindow {
protected:
	Array vstate_history;
	int vstate_history_pos;

		void add_vstate_history(ht_vstate_history_entry *e);
public:
	File	*file;

		UiFileWindow();

		void init(Bounds *b, const char *desc, uint framestyle, uint number, File *file);
	virtual	void done();
	/* overwritten */
	virtual	void handlemsg(htmsg *msg);
};

/*
 *	CLASS ht_project
 */

class ht_project: public AVLTree {
protected:
	char *filename;
public:
	explicit ht_project(const char *filename);
		ht_project(BuildCtorArg &a): AVLTree(a) {};
	virtual ~ht_project();
/* overwritten */
	virtual void load(ObjectStream &s);
	virtual ObjectID getObjectID() const;
	virtual void store(ObjectStream &s) const;
/* new */
		const char *get_filename();
};

/*
 *	CLASS ht_project_item
 */

class ht_project_item: public Object {
protected:
	char *filename;
	char *path;
public:
		ht_project_item(const char *filename, const char *path);
		ht_project_item(BuildCtorArg &a): Object(a) {};
	virtual ~ht_project_item();
/* overwritten */
	virtual void load(ObjectStream &s);
	virtual ObjectID getObjectID() const;
	virtual void store(ObjectStream &s) const;
	virtual int compareTo(const Object *) const;
/* new */
	const char *get_filename() const;
	const char *get_path() const;
};

/*
 *	CLASS UiProjectListbox
 */

class UiProjectListbox: public UiListbox {
protected:
	ht_project *project;
	uint colwidths[4];

public:
		void	init(Bounds *b, ht_project *project);

	virtual	int	calcCount();
	virtual	void	draw();
	virtual	void *	getFirst();
	virtual	void *	getLast();
	virtual	void *	getNext(void *entry);
	virtual	void *	getPrev(void *entry);
	virtual	const char *getStr(int col, void *entry);
	virtual	void	handlemsg(htmsg *msg);
	virtual	int	numColumns();
	virtual	void *	quickfind(const char *s);
	virtual	char *	quickfindCompletition(const char *s);
	virtual	bool	selectEntry(void *entry);
/* new */
		const char *func(uint i, bool execute);
		void	set_project(ht_project *project);
};

/*
 *	CLASS UiProjectWindow
 */

class UiProjectWindow: public UiWindow {
protected:
	ht_project **project;
	UiProjectListbox *plb;
	char wtitle[128];
public:

		void init(Bounds *b, const char *desc, uint framestyle, uint number, ht_project **project);
	virtual	void done();
/* overwritten */
	virtual	void handlemsg(htmsg *msg);
};

/*
 *	CLASS ht_app_window_entry
 */

#define AWT_LOG		0
#define AWT_CLIPBOARD	1
#define AWT_HELP	2
#define AWT_FILE	3
#define AWT_OFM		4
#define AWT_PROJECT	5

class ht_app_window_entry: public Object {
public:
	uint type;
	UiWindow *window;
	bool minimized;
	uint number;
	bool isfile;
	FileLayer *layer;

	ht_app_window_entry(UiWindow *window, uint number, uint type, bool minimized, bool isfile, FileLayer *layer);
	virtual int compareTo(const Object *) const;
};

/*
 *	CLASS ht_app
 */

class ht_app: public UiDialog {
protected:
	Container *windows;

	Container *syntax_lexers;

	UiKeyline *keyline;
	UiDesktop *desktop;

	UiGroup *battlefield;

	bool exit_program;

/* new */
			UiWindow *create_window_file_bin(Bounds *b, FileLayer *file, const char *title, bool isfile);
			UiWindow *create_window_file_text(Bounds *b, FileLayer *file, const char *title, bool isfile);

			bool accept_close_all_windows();
			uint find_free_window_number();

			uint get_window_number(UiWindow *window);
			ObjHandle get_window_listindex(UiWindow *window);

			void get_stdbounds_file(Bounds *b);
			void get_stdbounds_tool(Bounds *b);

			int popup_view_list_dump(UiView *view, UiTextListbox *listbox, List *structure, int depth, int *currenti, UiView *currentv);
/* overwritten */
	virtual	const char *defaultpalette();
	virtual	const char *defaultpaletteclass();
public:
	UiView *menu;

		ht_app() {};
		ht_app(BuildCtorArg &a): UiDialog(a) {};
		void insert_window(UiWindow *window, uint type, bool minimized, bool isfile, FileLayer *layer);

		void init(Bounds *b);
	virtual	void done();
/* overwritten */
	virtual	void draw();
	virtual	bool focus(UiView *view);
	virtual	const char *func(uint i, bool execute);
	virtual	void handlemsg(htmsg *msg);
	virtual	void load(ObjectStream &f);
	virtual ObjectID getObjectID() const;
	virtual	int run(bool modal);
	virtual	void store(ObjectStream &f) const;
/* new */
		UiWindow *create_window_clipboard();
		UiWindow *create_window_file(const char *filename, uint mode, bool allow_duplicates);
		UiWindow *create_window_file_bin(const char *filename, bool allow_duplicates);
		UiWindow *create_window_file_text(const char *filename, bool allow_duplicates);
		UiWindow *create_window_help(const char *file, const char *node);
		UiWindow *create_window_log();
		UiWindow *create_window_ofm(const char *url1, const char *url2);
		UiWindow *create_window_project();
		void delete_window(UiWindow *window);
		UiWindow *get_window_by_filename(const char *filename);
		UiWindow *get_window_by_number(uint number);
		UiWindow *get_window_by_type(uint type);
		UiView *popup_view_list(const char *dialog_title);
		UiWindow *popup_window_list(const char *dialog_title);
		void project_opencreate(const char *filename);
		void tile(bool vertical);
		void modal_resize();
};

extern ht_log *loglines;

/*
 *	INIT
 */

bool init_app();

/*
 *	DONE
 */

void done_app();

#endif /* __HTAPP_H__ */
