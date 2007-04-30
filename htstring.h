asdf
/* 
 *	HT Editor
 *	htstring.h
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

#ifndef __HTSTRING_H__
#define __HTSTRING_H__

#include "io/types.h"
#include "config.h"
#include "data.h"

char *ht_strdup(const char *str);
char *ht_strndup(const char *str, int maxlen);
int ht_strncpy(char *s1, const char *s2, int maxlen);
int ht_strncmp(const char *s1, const char *s2, size_t max);
int ht_strnicmp(const char *s1, const char *s2, size_t max);
int ht_stricmp(const char *s1, const char *s2);

int strcicomm(const char *s1, const char *s2);
int strccomm(const char *s1, const char *s2);
#define strend(s) ((s)+strlen(s))
int escape_special_str(char *result, int resultmaxlen, const char *s, const char *specialchars=0, bool bit7=true);
int escape_special(char *result, int resultmaxlen, const void *s, int len, const char *specialchars=0, bool bit7=true);
int unescape_special_str(char *result, int resultmaxlen, const char *s);
int unescape_special(void *result, int resultmaxlen, const char *s);
int bin2str(char *result, const void *s, int len);
void wide_char_to_multi_byte(char *result, const byte *unicode, int maxlen);

/* common string parsing functions */

void whitespaces(char **str);
bool waitforchar(char **str, char b);

/* string evaluation functions */

bool bnstr(char **str, uint32 *v, int defaultbase);
bool bnstr(char **str, uint64 *q, int defaultbase);

/* hex/string functions */

int hexdigit(char a);

bool hexb_ex(uint8 &result, const char *s);
bool hexw_ex(uint16 &result, const char *s);
bool hexd_ex(uint32 &result, const char *s);

char *mkhexb(char *buf, byte d);
char *mkhexw(char *buf, uint16 d);
char *mkhexd(char *buf, uint32 d);
char *mkhexq(char *buf, uint64 q);

/*
 *	ht_string_list
 */
class ht_string_list: public Array {
public:
		ht_string_list();
	/* new */
		const char *get_string(uint i);
		void insert_string(const char *s);
};

int compare_keys_string(Object *key_a, Object *key_b);
int icompare_keys_string(Object *key_a, Object *key_b);

/*
 *	INIT
 */

bool init_string();

/*
 *	DONE
 */

void done_string();

#endif /* !__HTSTRING_H__ */
