/*
 *	HT Editor
 *	io.h
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

#ifndef __IO_H__
#define __IO_H__

#include "types.h"

// include sys/types.h _before_ dirent.h because Darwin needs this (!)
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

/*
 *	COMMON SYS
 */
 

/* sys_ipc_exec(...) options param flags */
#define HT_IPC_NONBLOCKING	1

/* system capabilities */
#define SYSCAP_NONBLOCKING_IPC	1

/*
 *	COMMON CURSES
 */

/* virtual colors */

typedef int vc;

#define VC_BLACK			0
#define VC_BLUE			1
#define VC_GREEN			2
#define VC_CYAN			3
#define VC_RED				4
#define VC_MAGENTA			5
#define VC_YELLOW			6
#define VC_WHITE			7
#define VC_TRANSPARENT		8

#define VC_LIGHT(vc) ((vc) | 0x80)

#define VC_GET_LIGHT(vc) ((vc) & 0x80)
#define VC_GET_BASECOLOR(vc) ((vc) & 0x7f)

/* virtual color pairs (fg/bg) */

typedef int vcp;

#define VCP(vc_fg, vc_bg) (vcp)((vc_bg) | ((vc_fg)<<8))
#define VCP_BACKGROUND(v) ((v) & 0xff)
#define VCP_FOREGROUND(v) ((v>>8) & 0xff)

vcp vcp_mix(vcp base, vcp layer);

/*
 *	STRUCT drawbufch
 */

struct drawbufch {
	int ch;
	int c;
};

/*
 *	CLASS genericdrawbuf
 */

class genericdrawbuf {
public:
	bounds size;

	genericdrawbuf();
	virtual ~genericdrawbuf();
/* new */
	virtual void b_fill(int x, int y, int w, int h, int c, int ch);
		int b_printf(int x, int y, int c, char *format, ...);
	virtual int b_print(int x, int y, int c, char *text);
	virtual int b_printw(int x, int y, int c, int *text);
	virtual void b_printchar(int x, int y, int c, int ch);
	virtual int b_lprint(int x, int y, int c, int l, char *text);
	virtual int b_lprintw(int x, int y, int c, int l, int *text);
	virtual void b_resize(int rw, int rh);
	virtual void b_rmove(int rx, int ry);
	virtual void b_setbounds(bounds *b);
/* graphical extension */
	virtual void b_line(int px1, int py1, int px2, int py2, int c);
	virtual void b_putpixel(int px, int py, int c);
	virtual void text_to_pixel_coord(int tx, int ty, int *px, int *py);
	virtual void pixel_to_text_coord(int px, int py, int *tx, int *ty);
};

/*
 *	CLASS drawbuf
 */

class drawbuf: public genericdrawbuf {
public:
	drawbufch *buf;

	drawbuf(bounds *b);
	~drawbuf();
	
/* overwritten */
	virtual void b_fill(int x, int y, int w, int h, int c, int ch);
	virtual void b_printchar(int x, int y, int c, int ch);
	virtual int b_lprint(int x, int y, int c, int l, char *text);
	virtual int b_lprintw(int x, int y, int c, int l, int *text);
	virtual void b_setbounds(bounds *b);
};

/*
 *	COMMON KEYB
 */
 
enum ht_key {
K_FIRST = 0,

/* ASCII > 0 */

K_Backspace		= 1,
K_BackTab		= 2,

K_Tab			= '\t',
K_Return		= '\n',
K_Escape		= '\e',

K_Space			= ' ',
K_ExclamationPoint	= '!',
K_DoubleQuote		= '"',
K_Hash			= '#',
K_Dollar		= '$',
K_Percent		= '%',
K_Ampersand		= '&',
K_Quote			= '\'',
K_LParen		= '(',
K_RParen		= ')',
K_Star			= '*',
K_Plus			= '+',
K_Comma			= ',',
K_Dash			= '-',
K_Period		= '.',
K_Slash			= '/',
K_Colon			= ':',
K_SemiColon		= ';',
K_LAngle		= '<',
K_Equals		= '=',
K_RAngle		= '>',
K_QuestionMark		= '?',
K_At			= '@',

K_LBracket		= '[',
K_BackSlash		= '\'',
K_RBracket		= ']',
K_Caret			= '^',
K_UnderScore		= '_',
K_BackQuote		= '`',
K_LBrace		= '{',
K_Pipe			= '|',
K_RBrace		= '}',
K_Tilde			= '~',

K_BackTick		= '�',

K_0				= '0',
K_1				= '1',
K_2				= '2',
K_3				= '3',
K_4				= '4',
K_5				= '5',
K_6				= '6',
K_7				= '7',
K_8				= '8',
K_9				= '9',

K_A				= 'a',
K_B				= 'b',
K_C				= 'c',
K_D				= 'd',
K_E				= 'e',
K_F				= 'f',
K_G				= 'g',
K_H				= 'h',
K_I				= 'i',
K_J				= 'j',
K_K				= 'k',
K_L				= 'l',
K_M				= 'm',
K_N				= 'n',
K_O				= 'o',
K_P				= 'p',
K_Q				= 'q',
K_R				= 'r',
K_S				= 's',
K_T				= 't',
K_U				= 'u',
K_V				= 'v',
K_W				= 'w',
K_X				= 'x',
K_Y				= 'y',
K_Z				= 'z',

K_Shift_A			= 'A',
K_Shift_B			= 'B',
K_Shift_C			= 'C',
K_Shift_D			= 'D',
K_Shift_E			= 'E',
K_Shift_F			= 'F',
K_Shift_G			= 'G',
K_Shift_H			= 'H',
K_Shift_I			= 'I',
K_Shift_J			= 'J',
K_Shift_K			= 'K',
K_Shift_L			= 'L',
K_Shift_M			= 'M',
K_Shift_N			= 'N',
K_Shift_O			= 'O',
K_Shift_P			= 'P',
K_Shift_Q			= 'Q',
K_Shift_R			= 'R',
K_Shift_S			= 'S',
K_Shift_T			= 'T',
K_Shift_U			= 'U',
K_Shift_V			= 'V',
K_Shift_W			= 'W',
K_Shift_X			= 'X',
K_Shift_Y			= 'Y',
K_Shift_Z			= 'Z',

K_LASTASCII			= 0xff,

/* Special keys start here */

K_Alt_Escape,
K_Alt_Backspace,
K_Alt_Tab,
K_Alt_Enter,

K_Left,
K_Right,
K_Up,
K_Down,
K_PageUp,
K_PageDown,
K_Home,
K_End,
K_Insert,
K_Delete,

K_Control_Left,
K_Control_Right,
K_Control_Up,
K_Control_Down,
K_Control_PageUp,
K_Control_PageDown,
K_Control_Home,
K_Control_End,
K_Control_Insert,
K_Control_Delete,

K_Control_Shift_Left,
K_Control_Shift_Right,

K_Shift_Left,
K_Shift_Right,
K_Shift_Up,
K_Shift_Down,
K_Shift_PageUp,
K_Shift_PageDown,
K_Shift_Home,
K_Shift_End,
K_Shift_Insert,
K_Shift_Delete,

K_Control_A,
K_Control_B,
K_Control_C,
K_Control_D,
K_Control_E,
K_Control_F,
K_Control_G,
K_Control_H,
K_Control_I,
K_Control_J,
K_Control_K,
K_Control_L,
K_Control_M,
K_Control_N,
K_Control_O,
K_Control_P,
K_Control_Q,
K_Control_R,
K_Control_S,
K_Control_T,
K_Control_U,
K_Control_V,
K_Control_W,
K_Control_X,
K_Control_Y,
K_Control_Z,

K_Alt_A,
K_Alt_B,
K_Alt_C,
K_Alt_D,
K_Alt_E,
K_Alt_F,
K_Alt_G,
K_Alt_H,
K_Alt_I,
K_Alt_J,
K_Alt_K,
K_Alt_L,
K_Alt_M,
K_Alt_N,
K_Alt_O,
K_Alt_P,
K_Alt_Q,
K_Alt_R,
K_Alt_S,
K_Alt_T,
K_Alt_U,
K_Alt_V,
K_Alt_W,
K_Alt_X,
K_Alt_Y,
K_Alt_Z,

K_Alt_1,
K_Alt_2,
K_Alt_3,
K_Alt_4,
K_Alt_5,
K_Alt_6,
K_Alt_7,
K_Alt_8,
K_Alt_9,
K_Alt_0,

K_F1,
K_F2,
K_F3,
K_F4,
K_F5,
K_F6,
K_F7,
K_F8,
K_F9,
K_F10,
K_F11,
K_F12,

K_Shift_F1,
K_Shift_F2,
K_Shift_F3,
K_Shift_F4,
K_Shift_F5,
K_Shift_F6,
K_Shift_F7,
K_Shift_F8,
K_Shift_F9,
K_Shift_F10,
K_Shift_F11,
K_Shift_F12,

K_Control_F1,
K_Control_F2,
K_Control_F3,
K_Control_F4,
K_Control_F5,
K_Control_F6,
K_Control_F7,
K_Control_F8,
K_Control_F9,
K_Control_F10,
K_Control_F11,
K_Control_F12,

K_Alt_F1,
K_Alt_F2,
K_Alt_F3,
K_Alt_F4,
K_Alt_F5,
K_Alt_F6,
K_Alt_F7,
K_Alt_F8,
K_Alt_F9,
K_Alt_F10,
K_Alt_F11,
K_Alt_F12,

K_LAST,
K_INVALID = -1
};

#define K_COUNT			((int)K_LAST)

ht_key ht_metakey(ht_key key);		/* generate ht_key for M+key from key */
ht_key ht_unmetakey(ht_key metakey);	/* generate ht_key for key from M+key */

void ht_set_key(ht_key key, int rawkey);

ht_key ht_rawkey2key(int rawkey);

#endif /* __IO_H__ */

