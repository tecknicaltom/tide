/* 
 *	HT Editor
 *	x86dis.h
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf
 *	Copyright (C) 2005 Sebastian Biallas (sb@biallas.net)
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

#ifndef __X86DIS_H__
#define __X86DIS_H__

#include "asm.h"
#include "x86opc.h"

#define X86DIS_OPCODE_CLASS_STD		0		/* no prefix */
#define X86DIS_OPCODE_CLASS_EXT		1		/* 0F */
#define X86DIS_OPCODE_CLASS_EXT_F2	2		/* F2 0F */
#define X86DIS_OPCODE_CLASS_EXT_F3	3		/* F3 0F */
#define X86DIS_OPCODE_CLASS_EXTEXT	4		/* 0F 0F */

/* x86-specific styles */
#define X86DIS_STYLE_EXPLICIT_MEMSIZE	0x00000001	/* IF SET: mov word ptr [0000], ax 	ELSE: mov [0000], ax */
#define X86DIS_STYLE_OPTIMIZE_ADDR	0x00000002	/* IF SET: mov [eax*3], ax 		ELSE: mov [eax+eax*2+00000000], ax */

struct x86dis_insn {
	bool invalid;
	sint8 opsizeprefix;
	sint8 lockprefix;
	sint8 repprefix;
	sint8 segprefix;
	uint8 rexprefix;
	int size;
	int opcode;
	int opcodeclass;
	X86OpSize eopsize;
	X86AddrSize eaddrsize;
	char *name;
	x86_insn_op op[3];
};

/*
 *	CLASS x86dis
 */

class x86dis: public Disassembler {
public:
	X86OpSize opsize;
	X86AddrSize addrsize;

protected:
	x86dis_insn insn;
	char insnstr[256];
	unsigned char *codep, *ocodep;
	int seg;
	int addr; // FIXME: int??
	byte c;
	int modrm;
	int sib;
	int maxlen;

/* new */
			void	decode_insn(x86opc_insn *insn);
	virtual		void	decode_modrm(x86_insn_op *op, char size, bool allow_reg, bool allow_mem, bool mmx, bool xmm);
			void	decode_op(x86_insn_op *op, x86opc_insn_op *xop);
			void	decode_sib(x86_insn_op *op, int mod);
			int	esizeaddr(char c);
			int	esizeop(char c);
			byte	getbyte();
			uint16	getword();
			uint32	getdword();
			uint64	getqword();
			int	getmodrm();
			int	getsib();
			void	invalidate();
			bool	isfloat(char c);
			bool	isaddr(char c);
	virtual		void	prefixes();
			int	special_param_ambiguity(x86dis_insn *disasm_insn);
			void	str_format(char **str, char **format, char *p, char *n, char *op[3], int oplen[3], char stopchar, int print);
	virtual		void	str_op(char *opstr, int *opstrlen, x86dis_insn *insn, x86_insn_op *op, bool explicit_params);
			uint	mkmod(uint modrm);
			uint	mkreg(uint modrm);
			uint	mkindex(uint modrm);
			uint	mkrm(uint modrm);
public:
				x86dis(X86OpSize opsize, X86AddrSize addrsize);
				x86dis(BuildCtorArg&);

/* overwritten */
	virtual	dis_insn *	decode(byte *code, int maxlen, CPU_ADDR addr);
	virtual	dis_insn *	duplicateInsn(dis_insn *disasm_insn);
	virtual	void		getOpcodeMetrics(int &min_length, int &max_length, int &min_look_ahead, int &avg_look_ahead, int &addr_align);
	virtual	char *		getName();
	virtual	byte		getSize(dis_insn *disasm_insn);
		void		load(ObjectStream &f);
	virtual ObjectID	getObjectID() const;
	virtual char *		str(dis_insn *disasm_insn, int options);
	virtual char *		strf(dis_insn *disasm_insn, int options, char *format);
	virtual void		store(ObjectStream &f) const;
	virtual bool		validInsn(dis_insn *disasm_insn);
};

class x86_64dis: public x86dis {
public:	
				x86_64dis();
				x86_64dis(BuildCtorArg&);
	virtual		void	decode_modrm(x86_insn_op *op, char size, bool allow_reg, bool allow_mem, bool mmx, bool xmm);
	virtual		void	prefixes();
};

class x86dis_vxd: public x86dis {
protected:
	virtual void str_op(char *opstr, int *opstrlen, x86dis_insn *insn, x86_insn_op *op, bool explicit_params);
public:
				x86dis_vxd(BuildCtorArg&);
				x86dis_vxd(X86OpSize opsize, X86AddrSize addrsize);

	virtual dis_insn *	decode(byte *code, int maxlen, CPU_ADDR addr);
	virtual ObjectID	getObjectID() const;
};

#endif /* __X86DIS_H__ */
