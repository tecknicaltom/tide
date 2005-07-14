/*
 *	HT Editor
 *	infoview.cc
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

#include <stdlib.h>
#include <string.h>

#include "htdoc.h"
#include "htiobox.h"
#include "htpal.h"
#include "htstring.h"
#include "htsys.h"
#include "infoview.h"
#include "tools.h"

class info_pos: public Object {
public:
	uint line;
	uint ofs;
	
	info_pos(uint l, uint o) {
		line = l;
		ofs = o;
	}
};

int compare_keys_info_pos_delinear(Object *data_a, Object *data_b)
{
	info_pos *a = (info_pos*)data_a;
	info_pos *b = (info_pos*)data_b;
	uint32 da = delinearize(a->line);
	uint32 db = delinearize(b->line);
	if (da == db) {
		return delinearize(a->ofs) - delinearize(b->ofs);
	}
	return da - db;
}

int compare_keys_info_pos(Object *data_a, Object *data_b)
{
	info_pos *a = (info_pos*)data_a;
	info_pos *b = (info_pos*)data_b;
	if (a->line == b->line) {
		return a->ofs - b->ofs;
	}
	return a->line - b->line;
}

class info_xref: public Object {
public:
	char *target;
	uint len;
	
	info_xref(char *t, uint l) {
		target = strdup(t);
		len = l;
	}
	
	~info_xref() {
		free(target);
	}
};

// FIXME: this function is considered harmful
char *memndup(const char *s, int n)
{
	char *q = (char*)malloc(n+1);
	memcpy(q, s, n);
	q[n] = 0;
	return q;
}

/*
 *
 */

bool parse_xref_body(File *f, Container *t, char **n, uint *o, uint *line, bool note)
{
	whitespaces(n);
	char *l = strchr(*n, ':');
	if (!l) return false;
	char *e = l;
	while ((e>*n) && ((unsigned char)*(e-1)<=32)) e--;
	char *name = NULL;
	char *target = NULL;
	char *end = l;
	bool extrabreak=false;
	l++;
	whitespaces(&l);
	if (*(end+1) == ':') {
		name = memndup(*n, e-*n);
		end+=2;
	} else if ((note && (l-1 > end)) || (!note && (*(end+1) == ' '))){
		if (*(end+1) == '\n') extrabreak = true;
		char *v = l;
		if (*l == '(') {
			v = strchr(l, ')');
			if (!v) return false;
		}
		char *q = v;
		while (*q && (*q != '.') && (*q != ',')) q++;
		if (!*q) return false;
		char *p = q;

		while ((q>l) && ((unsigned char)*(q-1)<=32)) q--;
		name = memndup(*n, e-*n);
		target = memndup(l, q-l);
		end = p+1;
	} else return false;
	f->write(name, strlen(name));

//	fprintf(stderr, "xref: %s -> %s\n", name, ttt);

	char *thetarget = strdup(target ? target : name);
	char *ttt = thetarget;
	while (*ttt) {
		if (*ttt == '\n') *ttt = ' ';
		ttt++;
	}		

	char *p = name;
	info_xref *x = new info_xref(thetarget, *o);
	t->insert(new info_pos(*line, *o), x);
	while (*p) {
		if (*p=='\n') {
			x->len =  *o - x->len;
			*o = 0;
			(*line)++;
			x = new info_xref(thetarget, *o);
			t->insert(new info_pos(*line, *o), x);
		} else {
			(*o)++;
		}
		p++;
	}
	x->len =  *o - x->len;
	if (extrabreak) {
		char cr = '\n';
		f->write(&cr, 1);
		*o = 0;
		(*line)++;
	}
	free(thetarget);
	if (name) free(name);
	if (target) free(target);
//	fprintf(stderr, "t2\n");
	*n = end;
	return true;
}

ht_tree *parse_info_node(File *fl, char *infotext)
{
	char *n = infotext;
	bool linestart = true;
	FileOfs f = 0;
	uint o = 0;
	uint l = 0;
	ht_stree *t = new ht_stree();
	t->init(compare_keys_info_pos_delinear);

	while (*n && (*n != 0x1f)) {
		char *on = n;
		uint oo = o;
		uint ol = l;
		FileOfs of = f;
		char *k = (*n == '*') ? n : strchr(n, '*');
		if ((k == n) && (ht_strnicmp(n, "*note", 5) == 0)) {
			n += 5;
			if (!parse_xref_body(fl, t, &n, &o, &l, true)) {
				n = on;
				o = oo;
				l = ol;
				f = of;
				fl->seek(f);
				goto fallback;
			}
			f = fl->tell();
		} else if (linestart && (k == n) && (n[1] == ' ')) {
			n++;
			if (!parse_xref_body(fl, t, &n, &o, &l, false)) {
				n = on;
				o = oo;
				l = ol;
				f = of;
				fl->seek(f);
				goto fallback;
			}
			f = fl->tell();
		} else {
fallback:
			if (k && (k>n)) {
				char *cr = strchr(n, '\n');
				if (cr && (cr<k)) k = cr;
				if (k-n == 0) goto fallback2;
				fl->write(n, k-n);
				linestart = false;
				o+= k-n;
				f+= k-n;
				n+= k-n;
			} else {
fallback2:
				fl->write(n, 1);
				if (*n == '\n') {
					linestart = true;
					o=0;
					l++;
				} else {
					linestart = false;
					o++;
				}
				n++;
				f++;
			}
		}
	}
	t->set_compare_keys(compare_keys_info_pos);
	return t;
}
/*
\x1f\nFile:.*\n			lex_line = 0; lex_pofs = 0; file_block(&YY_LVALP->node, yytext); lex_ofs += yyleng; return INFO_FILE;
\x1f\nTag\ Table:.*\n		lex_ofs += strlen(yytext); return INFO_TTBL;
\x1f\nEnd\ Tag\ Table.*\n	lex_ofs += strlen(yytext); return INFO_ENDTTBL;
\*[nN]ote(\ |\n).[^:]*::	xref(&YY_LVALP->xref, yytext, 6); return INFO_XREF;
\*[nN]ote(\ |\n).[^:]*:\ ((\(.+\))|.)[^.,]*[.,]	xref(&YY_LVALP->xref, yytext, 6); return INFO_XREF;
^\*\ .[^:]*::			xref(&YY_LVALP->xref, yytext, 2); return INFO_XREF;
^\*\ .[^:]*:\ ((\(.+\))|.)[^.,]*[.,]	xref(&YY_LVALP->xref, yytext, 2); return INFO_XREF;
.				lex_ofs++; lex_pofs++; return yytext[0];
\n				lex_ofs++; lex_line++; lex_pofs=0; return yytext[0];
*/

/*
 *	CLASS ht_info_lexer
 */

void ht_info_lexer::init(ht_view *pf)
{
	ht_syntax_lexer::init();
	xrefs = NULL;
	cx = 0;
	cy = 0;
	pal_from = pf;
}

lexer_state ht_info_lexer::getinitstate()
{
	return 1;
}

vcp ht_info_lexer::getcolor_syntax(uint pal_index)
{
	return VCP(VC_WHITE, VC_BLACK);
}

lexer_token ht_info_lexer::geterrortoken()
{
	return 42;
}

char *ht_info_lexer::getname()
{
	return "infoview";
}

#define ILT_TEXT		1
#define ILT_LINK		2
#define ILT_LINK_SEL	3

lexer_token ht_info_lexer::gettoken(void *b, uint buflen, text_pos p, bool start_of_line, lexer_state *ret_state, uint *ret_len)
{
	if (buflen) {
		if (xrefs) {
			info_pos q(p.line, p.pofs);
			info_xref *x = (info_xref*)xrefs->get(&q);
			if (x) {
				*ret_len = MIN(x->len, buflen);
				return ((cy == p.line) && (cx >= p.pofs) &&
				(cx < p.pofs + x->len)) ? ILT_LINK_SEL : ILT_LINK;
			}
		}			
		*ret_len = 1;
		return ILT_TEXT;
	} else {
		*ret_len = 0;
		return 0;
	}
}

vcp ht_info_lexer::gettoken_color(lexer_token t)
{
	switch (t) {
		case ILT_TEXT:
			return pal_from->getcolor(palidx_generic_text_focused);
		case ILT_LINK:
			return pal_from->getcolor(palidx_generic_text_shortcut);
		case ILT_LINK_SEL:
			return pal_from->getcolor(palidx_generic_text_shortcut_selected);
	}
	return VCP(VC_WHITE, VC_RED);
}

void ht_info_lexer::set_cursor(uint x, uint y)
{
	cx = x;
	cy = y;
}

void ht_info_lexer::set_xrefs(ht_tree *x)
{
	xrefs = x;
}

/*
 *	CLASS ht_info_textfile
 */
 
class info_history_entry: public Object {
public:
	char *cwd;
	char *file;
	char *node;
	uint cursorx;
	uint cursory;
	uint xofs;
	uint top_line;

	info_history_entry(char *c, char *f, char *n, uint x, uint y, uint xo, uint yo)
	{
		cwd = ht_strdup(c);
		file = ht_strdup(f);
		node = ht_strdup(n);
		cursorx = x;
		cursory = y;
		xofs = xo;
		top_line = yo;
	}

	~info_history_entry()
	{
		if (cwd) free(cwd);
		if (file) free(file);
		if (node) free(node);
	}
};
 
void ht_info_textfile::init(File *s, bool own_s, ht_syntax_lexer *l)
{
	start = 0;
	end = 0;
	ht_ltextfile::init(s, own_s, l);
}

ht_ltextfile_line *ht_info_textfile::fetch_line(uint line)
{
	if (line < linecount())
		return ht_ltextfile::fetch_line(start+line);
	return NULL;
}

uint ht_info_textfile::linecount()
{
	return end-start;
}

void ht_info_textfile::set_node(uint ofs, uint len)
{
	uint s, e, t;
	start = 0;
	end = ht_ltextfile::linecount();
	convert_ofs2line(ofs, &s, &t);
	convert_ofs2line(ofs+len, &e, &t);
	start = s;
	end = e;
}

/*
 *	CLASS ht_info_viewer
 */

void ht_info_viewer::init(bounds *b)
{
	ht_mem_file *f = new ht_mem_file();
	f->init();

	ht_info_textfile *s = new ht_info_textfile();
	s->init(f, true, NULL);

	ht_text_viewer::init(b, true, s, NULL);
	cwd = NULL;
	file = NULL;
	node = NULL;
	xrefs = NULL;
	history = new ht_clist();
	((ht_clist*)history)->init();
}

void ht_info_viewer::done()
{
	if (history) {
		history->destroy();
		delete history;
	}
	if (xrefs) {
		xrefs->destroy();
		delete xrefs;
	}
	if (cwd) free(cwd);
	if (node) free(node);
	if (file) free(file);
	ht_text_viewer::done();
}

char *ht_info_viewer::defaultpalette()
{
	return palkey_generic_help_default;
}

void ht_info_viewer::draw()
{
	((ht_info_lexer*)lexer)->set_cursor(physical_cursorx(), cursory+top_line);
	ht_text_viewer::draw();
}

int ht_info_viewer::find_node(char *infotext, char *node)
{
	char *tags[] = {"File", "Node", "Prev", "Next", "Up"};
#define NUM_NODE_TAGS (sizeof (tags) / sizeof (tags[0]))
	char *s = infotext;
	char *firstnode = NULL;
	while ((s=strchr(s, 0x1f))) {
		s++;
		while ((*s>0) && (*s<32)) s++;
		char *cr = strchr(s, '\n');
		if (cr) {
			while (*s && (s<cr)) {
				whitespaces(&s);
				char *os = s;
				for (uint i=0; i<NUM_NODE_TAGS; i++) {
					uint l = strlen(tags[i]);
					if ((strncmp(s, tags[i], l) == 0) && (s[l] == ':')) {
						s += l+1;
						whitespaces(&s);
						char *e = strchr(s, ',');
						if (!e || (e>cr)) e = cr;
						if (!firstnode && (strcmp(tags[i], "Node") == 0)) {
							firstnode = cr+1;
						}
						if ((strcmp(tags[i], "Node") == 0) &&
						((size_t)(e-s) == strlen(node)) &&
						(strncmp(s, node, e-s)==0)) {
							return cr+1-infotext;
						}
						s = e+1;
					}
				}
				if (os == s) break;
			}
		}
	}
//     if (firstnode) return firstnode-infotext;
	return -1;
}

ht_tree *ht_info_viewer::get_xrefs()
{
	return xrefs;
}

bool ht_info_viewer::gotonode(char *f, char *n)
{
	return igotonode(f, n, true);
}

uint ht_info_viewer::readfile(char *fn, char **text)
{
	FILE *f = fopen(fn, "r");
	if (!f) return 0;
	
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *x = (char*)malloc(size+1);
	uint len = fread(x, 1, size, f);
	x[len] = 0;

	fclose(f);

	*text = x;
	return len;
}

bool ht_info_viewer::igotonode(char *f, char *n, bool add2hist)
{
	char *infotext = NULL;
	char nfile[HT_NAME_MAX];
	nfile[0] = 0;
	char ncwd[HT_NAME_MAX];
	ncwd[0] = 0;
	bool newnode = !node || (node && (strcmp(node, n) != 0));
	int fl = strlen(f)-strlen(MAGIC_HT_HELP);
	if ((fl>=0) && (strcmp(f+fl, MAGIC_HT_HELP) == 0)) {
		infotext = strdup(htinfo);
		strcpy(ncwd, "");
		strcpy(nfile, MAGIC_HT_HELP);
	} else {
		char ff[HT_NAME_MAX], cff[HT_NAME_MAX];
		cff[0] = 0;
		if (file && sys_common_canonicalize(cff, file, cwd, sys_is_path_delim) != 0) return false;
		if (sys_common_canonicalize(ff, f, cwd, sys_is_path_delim) != 0) return false;
		if ((strcmp(ff, cff) != 0) || newnode) {
			if (!readfile(ff, &infotext)) return false;
			char c[HT_NAME_MAX];
			if (sys_dirname(c, ff) == 0) strcpy(ncwd, c);
			if (sys_basename(c, ff) == 0) strcpy(nfile, c);
		}
	}
	if (infotext) {
		int o = find_node(infotext, n);
		if (o == -1) {
			free(infotext);
			return false;
		}

		ht_mem_file *m = new ht_mem_file();
		m->init();
		
		ht_tree *x = parse_info_node(m, infotext+o);
		if (x == NULL) {
			m->done();
			delete m;
			free(infotext);
			return false;
		}
		free(infotext);
		/* add to history or not*/
		if (add2hist && cwd && file && node) {
//			fprintf(stderr, "histhist: c:%s, f:%s, n:%s\n", cwd, file, node);
			history->insert(new info_history_entry(
				cwd, file, node, cursorx, cursory, 
				xofs, top_line));
		}			
		/* now modify text_viewer's state */
		if (xrefs) {
			xrefs->destroy();
			delete xrefs;
		}
		xrefs = x;

		ht_info_textfile *infofile=new ht_info_textfile();
		infofile->init(m, true, NULL);

		ht_info_lexer *infolexer = new ht_info_lexer();
		infolexer->init(this);

		infolexer->set_xrefs(xrefs);
		infofile->set_node(0, m->get_size());
		set_textfile(infofile, true);
		set_lexer(infolexer, true);

		cursorx = 0;
		cursory = 0;
		xofs = 0;
		top_line = 0;

		if (file) free(file);
		file = strdup(nfile);

		if (cwd) free(cwd);
		cwd = strdup(ncwd);

		if (node) free(node);
		node = strdup(n);

//		fprintf(stderr, "setset: c:%s, f:%s, n:%s\n", cwd, file, node);

		select_clear();
		return true;
	}
	return false;
}

void ht_info_viewer::get_pindicator_str(char *buf)
{
	buf += sprintf(buf, " %d:%d ", top_line+cursory+1, xofs+cursorx+1);
	sprintf(buf, "(%s) %s ", file, node);
}

void ht_info_viewer::handlemsg(htmsg *msg)
{
	if (msg->msg == msg_keypressed) {
		switch (msg->data1.integer) {
			case K_Space:
			case K_Return: 
				if (get_xrefs()) {
					info_pos p(top_line + cursory, xofs + physical_cursorx());
					info_xref *x = (info_xref*)get_xrefs()->get(&p);
					if (!x) {
						uint cx = physical_cursorx();
						info_pos *q = (info_pos*)get_xrefs()->enum_prev((ht_data**)&x, &p);
						if ((q) && ((q->line != top_line+cursory) ||
						(cx < q->ofs) || (cx >= q->ofs + x->len))) {
							x = NULL;
						}
					}
					if (x) {
						char *p = NULL, *q = NULL;
						char *a = x->target;
						if (*a == '(') {
							char *b = strchr(a, ')');
							if (b) {
								p = memndup(a+1, b-a-1);
								q = ht_strdup(b+1);
							}
						}
						if (!p)	p = ht_strdup(file);
						if (!q)	q = ht_strdup(x->target);
						if (!igotonode(p, q, true))
							errorbox("help topic '(%s)%s' not found", p, q);
						if (p) free(p);
						if (q) free(q);
					}
					clearmsg(msg);
					dirtyview();
					return;
				}
				break;
			case K_Alt_Backspace:
			case K_Backspace: {
				int c;
				if ((c = history->count())) {
					info_history_entry *e = (info_history_entry*)history->get(c-1);
//					fprintf(stderr, "backspace: %s, %s\n", e->file, e->node);
					if (e->node) {
						if (igotonode(e->file, e->node, false)) {
							cursorx = e->cursorx;
							cursory = e->cursory;
							xofs = e->xofs;
							top_line = e->top_line;
						} else {
							errorbox("help topic '(%s)%s' not found", e->file, e->node);
						}						
						history->del(c-1);
						clearmsg(msg);
						dirtyview();
						return;
					}						
				}
				break;					
			}
			case K_Tab: {
				if (get_xrefs()) {
					info_pos p(top_line + cursory, xofs + physical_cursorx());
					info_xref *r;
					info_pos *q = (info_pos*)get_xrefs()->enum_next((ht_data**)&r, &p);
					if (q) {
						goto_line(q->line);
						cursor_pput(q->ofs);
					}
				}					
				clearmsg(msg);
				dirtyview();
				return;
			}
			case K_BackTab: {
				if (get_xrefs()) {
					info_pos p(top_line + cursory, xofs + physical_cursorx());
					info_xref *r;
					info_pos *q = (info_pos*)get_xrefs()->enum_prev((ht_data**)&r, &p);
					if (q) {
						goto_line(q->line);
						cursor_pput(q->ofs);
					}
				}					
				clearmsg(msg);
				dirtyview();
				return;
			}
		}
	}
	ht_text_viewer::handlemsg(msg);
}
