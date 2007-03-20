/*
 *	HT Editor
 *	out_html.cc
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

#include "analy_names.h"
#include "htdebug.h"
#include "htinfo.h"
#include "out_html.h"
#include "tools.h"
#include "x86dis.h"
#include "string.h"
#include "snprintf.h"

static const char *header_str = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n<html>\n<head>\n";
static const char *footer_str = "</body>\n</html>\n";
static const char *stylesheet_str = "<style type=\"text/css\">\n"
"<!--\n"
"body {\n"
"\tfont: 10pt arial,helvetica,sans-serif;\n"
"\tcolor:#000000;\n"
"\tbackground:#ffffff;\n"
"}\n"
"A:link {\n"
"\tcolor:#0000ff;\n"
"\ttext-decoration: none;\n"
"}\n"
"A:active {\n"
"\tcolor:#ff00ff;\n"
"\ttext-decoration: none;\n"
"}\n"
"A:visited {\n"
"\tcolor:#0000ff;\n"
"\ttext-decoration: none;\n"
"}\n"
"td {\n"
"\tfont: 12pt courier;\n"
"\tcolor:#000000;\n"
"\tvertical-align: top;\n"
"}\n"
"b {\n"
"\tfont: bold 11pt arial,helvetica,sans-serif;\n"
"\tcolor:#000000;\n"
"}\n"
"//-->\n"
"</style>\n";

#if 0
int write_str(Stream *stream, char *s)
{
	return stream->write(s, strlen(s));
}

Analyser *theanaly2;
char html_buffer[1024];

char *html_addr_sym_func(CPU_ADDR Addr, int *symstrlen)
{
	/* should only be called/used/assigned if theanaly2 is set */
	Location *a = theanaly2->getLocationByAddress(Addr.addr32.offset);
	if (a && a->label){
		int i = sprintf(html_buffer, "<a href=\"#A%08X\">%s</a>", Addr.addr32.offset, a->label->name);
		if (symstrlen) *symstrlen=i;
		return html_buffer;
	} else {
		if (theanaly2->valid_addr(Addr.addr32.offset, scvalid)) {
			int i = sprintf(html_buffer, "<a href=\"#A%08X\">%x</a>", Addr.addr32.offset, Addr.addr32.offset);
			if (symstrlen) *symstrlen=i;
			return html_buffer;
		}
	}
	return NULL;
}

int generate_html_output(Analyser *analy, Stream *stream, Address *from, Address *to)
{
	if (!analy || !stream) return HTML_OUTPUT_ERR_GENERIC;
	if (analy->active) return HTML_OUTPUT_ERR_ANALY_NOT_FINISHED;

	write_str(stream, header);
	write_str(stream, "\t<title>Analysis of ");
	write_str(stream, analy->getname());
	write_str(stream, "</title>\n");
	write_str(stream, stylesheet);
	write_str(stream, "</head>\n<body bgcolor=\"#ffffff\">\n");
	write_str(stream, "Analysis of <i>");
	write_str(stream, analy->getname());
	write_str(stream, "</i><br>generated by <a href=\""ht_url"\">"ht_name" version "ht_version"</a>.\n<hr>\n");
	write_str(stream, "\n<table cellspacing=0 border=0>\n");
	// main loop:
	ADDR Addr = from;
	while (1) {
		if (Addr > to) break;
		write_str(stream, "\n<tr>\n");

		taddr *a = analy->findaddr(Addr);

		int length = 0;

		char temp[1024];

		sprintf(temp, "<td><a name=\"A%08X\">"HEX8FORMAT8"</a></td>\n", Addr, Addr);
		write_str(stream, temp);


		if (analy->explored->contains(Addr)) {
			write_str(stream, "<td bgcolor=black>&nbsp;</td>\n");
		} else {
			write_str(stream, "<td>&nbsp;</td>\n");
		}


		if (a) {
			write_str(stream, "<td>\n<table cellspacing=0 border=0>\n");
			// comments
			tcomment *c = a->comments;
			if (c) {
				write_str(stream, "<tr><td colspan=2>\n");
				while (c)	{
					write_str(stream, c->text);
					write_str(stream, "<br>\n");
					c = c->next;
				}
				write_str(stream, "</td></tr>\n");
			}
			// label
			write_str(stream, "<tr>\n<td width=500>");
			if (a->label) {
				sprintf(temp, "<b>%s</b>:<br>\n", a->label->name);
				write_str(stream, temp);
			}
		} else {
			write_str(stream, "<td width=500>");
		}

		if (analy->validcodeaddr(Addr)) {
			taddr *nextaddr = analy->enum_addrs(Addr);
			int op_len;

			if (nextaddr) {
				op_len = MIN((uint32)analy->maxopcodelength, (nextaddr->addr - Addr));
			} else {
				op_len = analy->maxopcodelength;
			}

			if (analy->disasm) {
				byte buf[16];
				OPCODE *o=analy->disasm->decode(analy->bufptr(Addr, buf, sizeof(buf)), op_len, analy->mapaddr(Addr));
				/* inits for addr-sym transformations */
				theanaly2 = analy;
				addr_sym_func = &html_addr_sym_func;
				/**/
				sprintf(temp, "  %s", analy->disasm->str(o, X86DIS_STYLE_HEX_NOZEROPAD));
				/* deinits for addr-sym transformations */
				theanaly2 = NULL;
				addr_sym_func = 0;
				/**/
				length=analy->disasm->getsize(o);
			} else {
				byte c;
				sprintf(temp, "  db      0%02xh", *(byte *)analy->bufptr(Addr, &c, 1));
				length=1;
			}
		} else {
			if (analy->validaddr(Addr, scvalid)) {
				if ((a) && (a->type.type == dtint)) {
					length = a->type.length;
					assert(length);
					byte c[4];
					if (analy->validaddr(Addr, scinitialized)) {
						switch (a->type.intsubtype) {
							case dstiword:
								sprintf(temp, "  dw      0%04xh", *(uint16 *)analy->bufptr(Addr, c, 2));
								break;
							case dstidword:
								sprintf(temp, "  dd      0%08xh", *(uint32 *)analy->bufptr(Addr, c, 4));
								break;
							case dstibyte:
							default:
								analy->bufptr(Addr, c, 1);
								sprintf(temp, "  db      %02xh ; '%c'", c[0], (c[0]<32)?32:c[0]);
						}
					} else {
						switch (a->type.intsubtype) {
							case dstiword:
								strcpy(temp, "  dw      ????");
								break;
							case dstidword:
								strcpy(temp, "  dd      ????????");
								break;
							case dstibyte:
							default:
								strcpy(temp, "  db      ??");
						}
					}
				} else {
					length =	1;
					if (analy->validaddr(Addr, scinitialized)) {
						byte c;
						analy->bufptr(Addr, &c, 1);
						sprintf(temp, "  db      0%02xh ; '%c'", c, (c<32)?32:c);
					} else {
						strcpy(temp, "  db      ??");
					}
				}
			} else {
				ADDR next = analy->nextvalid(Addr);
				if (next != INVALID_ADDR) {
					length = next - Addr;
					sprintf(temp, "  db      ?? * %d", next - Addr);
				} else {
					length =	1;
					strcpy(temp, "  db      ??");
				}
			}
		}
/*		if (mode & ANALY_EDIT_BYTES) {
			if (validaddr(Addr, scinitialized)) {

				FILEADDR a=fileaddr(Addr);
				assert(a != INVALID_FILE_OFS);
				for (int i=0; i<(*length); i++) {
					buf = tag_make_edit_byte(buf, a+i);
				}
				for (int i=0; i<=(maxopcodelength*2)-(*length)*2; i++) {
					*(buf++)=' ';
				}
				*buf=0;
			}
		}*/

		char *t = temp;
		while (*t) {
			if ((t[0]==' ') && (t[1]==' ')) {
				write_str(stream, "&nbsp;");
			} else {
				stream->write(t, 1);
			}
			t++;
		}

		if (a) {
			write_str(stream, "</td>\n<td>");
			txref *x = a->xreflist;
			int i=0;
			while (x)	{
				if (i % 3==0) {
					write_str(stream, ";xref");
				}
				sprintf(temp, " %c<a href=\"#A%08X\">"HEX8FORMAT"</a>", xref_type_short(x->type), x->addr, x->addr);
				write_str(stream, temp);
				if (i % 3==2) {
					write_str(stream, "<br>\n");
				}
				i++;
				x = x->next;
			}
			write_str(stream, "</td>\n</tr></table>");
		}
		write_str(stream, "</td>\n</tr>\n");

		Addr += length;
	}
	// end
	write_str(stream, "</table>\n");
	write_str(stream, footer);
	return HTML_OUTPUT_OK;
	return HTML_OUTPUT_OK;
}
#endif

// #################################################################

void AnalyserHTMLOutput::init(Analyser *analy, Stream *s)
{
	AnalyserOutput::init(analy);
	stream = s;
	dis_style = DIS_STYLE_HEX_NOZEROPAD+DIS_STYLE_HEX_ASMSTYLE+X86DIS_STYLE_OPTIMIZE_ADDR;
}

void AnalyserHTMLOutput::beginAddr()
{
	AnalyserOutput::beginAddr();
}

void AnalyserHTMLOutput::beginLine()
{
	AnalyserOutput::beginLine();
	char temp[20];
	if (line == 0) {
		addr->stringify(temp, sizeof temp, ADDRESS_STRING_FORMAT_LEADING_WHITESPACE);
		char temp2[20];
		last = addr->stringify(temp2, sizeof temp2, ADDRESS_STRING_FORMAT_COMPACT);
	} else {
		int s = addr->stringSize();
		memset(temp, ' ', s);
		memset(&temp[s-last], '.', last);
		temp[s] = 0;
	}
	write(temp);
	
	if (analy->explored->contains(addr)) {
		write(" ! ");
	} else {
		write("   ");
	}
}

Stream *AnalyserHTMLOutput::getGenerateStream()
{
	return stream;
}

int AnalyserHTMLOutput::elementLength(const char *s)
{
	return strlen(s);
}

void AnalyserHTMLOutput::endAddr()
{
	AnalyserOutput::endAddr();
}

void AnalyserHTMLOutput::endLine()
{
	write("\n");
	AnalyserOutput::endLine();
}

void AnalyserHTMLOutput::putElement(int element_type, const char *element)
{
	switch (element_type) {
	case ELEMENT_TYPE_HIGHLIGHT_DATA_CODE:
		write("  ");
		write(element);
		break;
	default:
		write(element);
		break;
	}
}

char *AnalyserHTMLOutput::link(char *s, Address *Addr)
{
	global_analyser_address_string_format = ADDRESS_STRING_FORMAT_LEADING_ZEROS | ADDRESS_STRING_FORMAT_ADD_H;
	ht_snprintf(tmpbuf, sizeof tmpbuf, "<a href=\"#%y\">%s</a>", Addr, s);
	return tmpbuf;
}

char *AnalyserHTMLOutput::externalLink(char *s, uint32 type1, uint32 type2, uint32 type3, uint32 type4, void *special)
{
	strcpy(tmpbuf, "test");
	return tmpbuf;
}

static void swrite(Stream *s, const char *str)
{
	s->writex(str, strlen(str));
}

void AnalyserHTMLOutput::footer()
{
	swrite(stream, footer_str);
}

void AnalyserHTMLOutput::header()
{
	String name;
	analy->getName(name);
	swrite(stream, header_str);
	swrite(stream, "\t<title>Analysis of ");
	swrite(stream, name.contentChar());
	swrite(stream, "</title>\n");
	swrite(stream, stylesheet_str);
	swrite(stream, "</head>\n<body bgcolor=\"#ffffff\">\n\n");
	swrite(stream, "Analysis of <i>");
	swrite(stream, name.contentChar());
	swrite(stream, "</i><br>generated by <a href=\""ht_url"\">"ht_name" version "ht_version"</a>.\n<hr>\n<pre>\n");
}
