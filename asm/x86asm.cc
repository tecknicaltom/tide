/*x
 *	HT Editor
 *	x86asm.cc
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf
 *	Copyright (C) 2005-2006 Sebastian Biallas (sb@biallas.net)
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

#include "x86asm.h"
#include "snprintf.h"
#include "strtools.h"

#define X86ASM_PREFIX_NO			0
#define X86ASM_PREFIX_0F			1
#define X86ASM_PREFIX_F20F			2
#define X86ASM_PREFIX_F30F			3
#define X86ASM_PREFIX_0F0F			4
#define X86ASM_PREFIX_0F38			5
#define X86ASM_PREFIX_0F3A			6
#define X86ASM_PREFIX_D8			7
#define X86ASM_PREFIX_D9			8
#define X86ASM_PREFIX_DA			9
#define X86ASM_PREFIX_DB			10
#define X86ASM_PREFIX_DC			11
#define X86ASM_PREFIX_DD			12
#define X86ASM_PREFIX_DE			13
#define X86ASM_PREFIX_DF			14

#define X86ASM_ERRMSG_AMBIGUOUS		"ambiguous command"
#define X86ASM_ERRMSG_UNKNOWN_COMMAND	"unknown command '%s'"
#define X86ASM_ERRMSG_INVALID_PREFIX	"invalid prefix"
#define X86ASM_ERRMSG_UNKNOWN_SYMBOL	"unknown symbol '%s'"
#define X86ASM_ERRMSG_INVALID_OPERANDS	"invalid operand(s)"
#define X86ASM_ERRMSG_INTERNAL		"internal error: "

#define rexw 0x48
#define rexr 0x44
#define rexx 0x42
#define rexb 0x41

static const x86addrcoding modrm16[3][8] = {
/* mod = 0 */
{
{X86_REG_BX, X86_REG_SI, 0},
{X86_REG_BX, X86_REG_DI, 0},
{X86_REG_BP, X86_REG_SI, 0},
{X86_REG_BP, X86_REG_DI, 0},
{X86_REG_SI, X86_REG_NO, 0},
{X86_REG_DI, X86_REG_NO, 0},
{X86_REG_NO, X86_REG_NO, 2},
{X86_REG_BX, X86_REG_NO, 0}
},
/* mod = 1 */
{
{X86_REG_BX, X86_REG_SI, 1},
{X86_REG_BX, X86_REG_DI, 1},
{X86_REG_BP, X86_REG_SI, 1},
{X86_REG_BP, X86_REG_DI, 1},
{X86_REG_SI, X86_REG_NO, 1},
{X86_REG_DI, X86_REG_NO, 1},
{X86_REG_BP, X86_REG_NO, 1},
{X86_REG_BX, X86_REG_NO, 1}
},
/* mod = 2 */
{
{X86_REG_BX, X86_REG_SI, 2},
{X86_REG_BX, X86_REG_DI, 2},
{X86_REG_BP, X86_REG_SI, 2},
{X86_REG_BP, X86_REG_DI, 2},
{X86_REG_SI, X86_REG_NO, 2},
{X86_REG_DI, X86_REG_NO, 2},
{X86_REG_BP, X86_REG_NO, 2},
{X86_REG_BX, X86_REG_NO, 2}
}
};

static const x86addrcoding modrm32[3][8] = {
/* mod = 0 */
{
{X86_REG_AX, X86_REG_NO, 0},
{X86_REG_CX, X86_REG_NO, 0},
{X86_REG_DX, X86_REG_NO, 0},
{X86_REG_BX, X86_REG_NO, 0},
{X86_REG_INVALID, X86_REG_INVALID, -1},		/* special: SIB */
{X86_REG_NO, X86_REG_NO, 4},
{X86_REG_SI, X86_REG_NO, 0},
{X86_REG_DI, X86_REG_NO, 0}
},
/* mod = 1 */
{
{X86_REG_AX, X86_REG_NO, 1},
{X86_REG_CX, X86_REG_NO, 1},
{X86_REG_DX, X86_REG_NO, 1},
{X86_REG_BX, X86_REG_NO, 1},
{X86_REG_INVALID, X86_REG_INVALID, -1},		/* special: SIB + disp8 */
{X86_REG_BP, X86_REG_NO, 1},
{X86_REG_SI, X86_REG_NO, 1},
{X86_REG_DI, X86_REG_NO, 1}
},
/* mod = 2 */
{
{X86_REG_AX, X86_REG_NO, 4},
{X86_REG_CX, X86_REG_NO, 4},
{X86_REG_DX, X86_REG_NO, 4},
{X86_REG_BX, X86_REG_NO, 4},
{X86_REG_INVALID, X86_REG_INVALID, -1},		/* special: SIB + disp32 */
{X86_REG_BP, X86_REG_NO, 4},
{X86_REG_SI, X86_REG_NO, 4},
{X86_REG_DI, X86_REG_NO, 4}
}
};

/* convert logical operand types to hardware operand types */
static const int lop2hop[12][8] = {
	/* X86_OPTYPE_EMPTY */
	{},
	/* X86_OPTYPE_IMM */
	{TYPE_I, TYPE_Is, TYPE_J, TYPE_A, TYPE_Ix},
	/* X86_OPTYPE_REG */
	{TYPE_R, TYPE_Rx, TYPE_RXx, TYPE_G, TYPE_E},
	/* X86_OPTYPE_SEG */
	{TYPE_S, TYPE_Sx},
	/* X86_OPTYPE_MEM */
	{TYPE_E, TYPE_M, TYPE_O, TYPE_Q, TYPE_W},
	/* X86_OPTYPE_CRX */
	{TYPE_C},
	/* X86_OPTYPE_DRX */
	{TYPE_D},
	/* X86_OPTYPE_TRX */
	{TYPE_T},
	/* X86_OPTYPE_STX */
	{TYPE_F, TYPE_Fx},
	/* X86_OPTYPE_MMX */
	{TYPE_P, TYPE_Q, TYPE_PR},
	/* X86_OPTYPE_XMM */
	{TYPE_V, TYPE_W, TYPE_VR},
	/* X86_OPTYPE_FARPTR */
	{},
};

static const char immhsz8_16[] = { SIZE_B, SIZE_BV, SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz16_16[] = { SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz32_16[] = { 0 };
static const char immhsz64_16[] = { 0 };

static const char immhsz8_32[] = { SIZE_B, SIZE_BV, SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz16_32[] = { SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz32_32[] = { SIZE_V, SIZE_VV, 0 };
static const char immhsz64_32[] = { 0 };

static const char immhsz8_64[] = { SIZE_B, SIZE_BV, SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz16_64[] = { SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char immhsz32_64[] = { SIZE_V, SIZE_VV, 0 };
static const char immhsz64_64[] = { SIZE_V, 0 };

static const char hsz8_16[] = { SIZE_B, 0 };
static const char hsz16_16[] = { SIZE_W, SIZE_V, SIZE_VV, 0 };
static const char hsz32_16[] = { SIZE_D, SIZE_P, SIZE_Z, 0 };
static const char hsz48_16[] = { 0 };
static const char hsz64_16[] = { SIZE_Q, SIZE_U, SIZE_Z, 0};
static const char hsz128_16[] = { SIZE_O, SIZE_U, 0};

static const char hsz8_32[] = { SIZE_B, 0 };
static const char hsz16_32[] = { SIZE_W, 0 };
static const char hsz32_32[] = { SIZE_D, SIZE_V, SIZE_VV, SIZE_R, SIZE_Z, 0 };
static const char hsz48_32[] = { SIZE_P, 0 };
static const char hsz64_32[] = { SIZE_Q, SIZE_U, SIZE_Z, 0};
static const char hsz128_32[] = { SIZE_O, SIZE_U, 0};

static const char hsz8_64[] = { SIZE_B, 0 };
static const char hsz16_64[] = { SIZE_W, 0 };
static const char hsz32_64[] = { SIZE_D, SIZE_Z, 0 };
static const char hsz48_64[] = { 0 };
static const char hsz64_64[] = { SIZE_Q, SIZE_U, SIZE_V, SIZE_VV, SIZE_R, SIZE_Z, 0};
static const char hsz128_64[] = { SIZE_O, SIZE_U, 0};

static const int reg2size[4] = {1, 2, 4, 8};

/*
 *	CLASS x86asm
 */

x86asm::x86asm(int o, int a)
: Assembler(false)
{
	opsize = o;
	addrsize = a;
	x86_insns = &x86_32_insns;
}

asm_insn *x86asm::alloc_insn()
{
	return ht_malloc(sizeof (x86asm_insn));
}

void x86asm::delete_nonsense()
{
restart:
	asm_code *c=codes;
	while (c) {
		if (delete_nonsense_insn(c)) goto restart;
		c=c->next;
	}
}

bool x86asm::delete_nonsense_insn(asm_code *code)
{
	byte *d = code->data;
	int size = code->size;
	while (*d==0x66 || *d==0x67) {
		d++;
		size--;
	}
	asm_code *c = codes;
	while (c) {
		if ((bool)c->context == 0) {
			byte *cd = c->data;
			int csize = c->size;
			while (*cd == 0x66 || *cd == 0x67) {
				cd++;
				csize--;
			}
			if (c->size < code->size && size == csize) {
				if (memcmp(d, cd, size) == 0) {
					deletecode(code);
					return true;
				}
			}
		}
		c = c->next;
	}
	return false;
}

void x86asm::emitdisp(uint64 d, int size)
{
	dispsize = size;
	disp = d;
}

void x86asm::emitimm(uint64 i, int size)
{
	immsize = size;
	imm = i;
}

void x86asm::emitfarptr(uint32 s, uint32 o, bool big)
{
	if (big) {
		immsize = 6;
		imm = o;
		imm2 = s;
	} else {
		immsize = 4;
		imm = (s<<16) | (o & 0xffff);
	}
}

void x86asm::emitmodrm(int modrm)
{
	modrmv = modrm;
}

void x86asm::emitmodrm_mod(int mod)
{
	if (modrmv == -1) modrmv = 0;
	modrmv = (modrmv & ~(3<<6)) | ((mod & 3)<<6);
}

void x86asm::emitmodrm_reg(int reg)
{
	if (modrmv == -1) modrmv = 0;
	modrmv = (modrmv & ~(7<<3)) | ((reg & 7)<<3);
}

void x86asm::emitmodrm_rm(int rm)
{
	if (modrmv == -1) modrmv = 0;
	modrmv = (modrmv & ~7) | (rm & 7);
}

void x86asm::emitsib_base(int base)
{
	if (sibv == -1) sibv = 0;
	sibv = (sibv & ~7) | (base & 7);
}

void x86asm::emitsib_index(int index)
{
	if (sibv == -1) sibv = 0;
	sibv = (sibv & ~(7<<3)) | ((index & 7)<<3);
}

void x86asm::emitsib_scale(int scale)
{
	if (sibv == -1) sibv = 0;
	sibv = (sibv & ~(3<<6)) | ((scale & 3)<<6);
}

asm_code *x86asm::encode(asm_insn *asm_insn, int options, CPU_ADDR cur_address)
{
	Assembler::encode(asm_insn, options, cur_address);
	x86asm_insn *insn = (x86asm_insn*)asm_insn;
	
	addrsize_depend = false;
	for (int i=0; i < 3; i++) {
		if (insn->op[i].type == X86_OPTYPE_MEM) {
			addrsize_depend = true;
			break;
		}
	}
		
	newcode();
	namefound = false;
	if (addrsize == X86_ADDRSIZE64) {
		address = cur_address.flat64.addr;
	} else {
		address = cur_address.addr32.offset;
	}
	esizes[0] = 0;
	esizes[1] = 0;
	esizes[2] = 0;
	ambiguous = false;
	match_opcodes(*x86_insns, insn, X86ASM_PREFIX_NO);
	if (!namefound && insn->repprefix != X86_PREFIX_NO) {
		set_error_msg(X86ASM_ERRMSG_INVALID_PREFIX);
	} else {
		match_fopcodes(insn);
        	match_opcodes(x86_32_insns_ext, insn, X86ASM_PREFIX_0F);
        	match_opcodes(x86_insns_ext_f2, insn, X86ASM_PREFIX_F20F);
        	match_opcodes(x86_insns_ext_f3, insn, X86ASM_PREFIX_F30F);
        	match_opcodes(x86_opc_group_insns[0], insn, X86ASM_PREFIX_0F38);
        	match_opcodes(x86_opc_group_insns[1], insn, X86ASM_PREFIX_0F3A);
	}
	if (error) {
		free_asm_codes();
	} else if (!codes) {
		if (namefound) {
			set_error_msg(X86ASM_ERRMSG_INVALID_OPERANDS);
		} else {
			set_error_msg(X86ASM_ERRMSG_UNKNOWN_COMMAND, insn->name);
		}
	} else {
		delete_nonsense();
	}
	return codes;
}

bool x86asm::encode_insn(x86asm_insn *insn, x86opc_insn *opcode, int opcodeb, int additional_opcode, int prefix, int eopsize, int eaddrsize)
{	
	rexprefix = 0;
	disppos = 0;

	bool opsize_depend = false;
	for (int i = 0; i < 3; i++) {
		switch (opcode->op[i].size) {
		case SIZE_BV:
		case SIZE_V:
		case SIZE_VV:
		case SIZE_P:
			opsize_depend = true;
			break;
		}
	}

	code.context = (void*)opsize_depend;
	
	/* test rex thingies */
	for (int i=0; i<3; i++) {
		if (insn->op[i].need_rex) {
			rexprefix |= 0x40;
		}
		if (insn->op[i].forbid_rex) {
			rexprefix |= 0x80;
		}
	}

	modrmv = -1;
	sibv = -1;
	dispsize = 0;
	immsize = 0;
	if (additional_opcode != -1) {
		if (additional_opcode & 0x800) {
			emitmodrm_mod(3);
			emitmodrm_reg(additional_opcode & 0x7);
			emitmodrm_rm((additional_opcode >> 3) & 0x7);
		} else {
			emitmodrm_reg(additional_opcode);
		}
	}

	if (addrsize == X86_ADDRSIZE64) {		
		if (eopsize == X86_ADDRSIZE64) {
			if (insn->opsizeprefix == X86_PREFIX_OPSIZE) emitbyte(0x66);
			if (!(opcode->op[0].info & 0x80)) {
				// instruction doesn't default to 64 bit opsize
				rexprefix |= rexw;
			}
		} else if (eopsize == X86_ADDRSIZE32) {
			if (opcode->op[0].info & 0x80) {
				// instruction defaults to 64 bit opsize
				// it's not possible to switch to 32 bit
				return false;
			}
			if (insn->opsizeprefix == X86_PREFIX_OPSIZE) emitbyte(0x66);
		} else if (eopsize == X86_ADDRSIZE16) {
			emitbyte(0x66);
		}
		if (eaddrsize == X86_ADDRSIZE16) return false;
		if (eaddrsize == X86_ADDRSIZE32) emitbyte(0x67);
	} else {
		if (eopsize != opsize || insn->opsizeprefix == X86_PREFIX_OPSIZE) {
			emitbyte(0x66);
		}
		if (eaddrsize != addrsize) emitbyte(0x67);
	}
	
	if ((rexprefix & 0xc0) == 0xc0) {
		// can't combine insns which simultaneously need REX and forbid REX
		clearcode();
		return false;
	}

	/* write lock, rep and/or seg prefixes if needed */
	switch (insn->lockprefix) {
		case X86_PREFIX_LOCK: emitbyte(0xf0); break;
	}
	switch (insn->repprefix) {
		case X86_PREFIX_REPNZ: emitbyte(0xf2); break;
		case X86_PREFIX_REPZ: emitbyte(0xf3); break;
	}
	switch (insn->segprefix) {
	case X86_PREFIX_ES: emitbyte(0x26); break;
	case X86_PREFIX_CS: emitbyte(0x2e); break;
	case X86_PREFIX_SS: emitbyte(0x36); break;
	case X86_PREFIX_DS: emitbyte(0x3e); break;
	case X86_PREFIX_FS: emitbyte(0x64); break;
	case X86_PREFIX_GS: emitbyte(0x65); break;
	}

	switch (prefix) {
	case X86ASM_PREFIX_F20F:
		emitbyte(0xf2);
		break;
	case X86ASM_PREFIX_F30F:
		emitbyte(0xf3);
		break;
	}

	int rexpos = code.size;
	if (rexprefix & 0x40) emitbyte(0xff); // dummy value
	
	/* write opcodeprefixes and opcode */
	switch (prefix) {
	case X86ASM_PREFIX_0F0F:
		emitbyte(0x0f);
	case X86ASM_PREFIX_F20F:
	case X86ASM_PREFIX_F30F:
	case X86ASM_PREFIX_0F:
		emitbyte(0x0f);
	case X86ASM_PREFIX_NO:
		break;
	case X86ASM_PREFIX_0F38:
		emitbyte(0x0f);
		emitbyte(0x38);
		break;
	case X86ASM_PREFIX_0F3A:
		emitbyte(0x0f);
		emitbyte(0x3a);
		break;
	case X86ASM_PREFIX_DF:
	case X86ASM_PREFIX_DE:
	case X86ASM_PREFIX_DD:
	case X86ASM_PREFIX_DC:
	case X86ASM_PREFIX_DB:
	case X86ASM_PREFIX_DA:
	case X86ASM_PREFIX_D9:
	case X86ASM_PREFIX_D8:
		opcodeb = 0xd8 + prefix - X86ASM_PREFIX_D8;
		break;
	}
	emitbyte(opcodeb);

	/* encode the ops */
	for (int i=0; i<3; i++) {
		if (!encode_op(&insn->op[i], &opcode->op[i], &esizes[i], eopsize, eaddrsize)) {
			clearcode();
			return false;
		}
	}

	/* write the rest */
	if (modrmv != -1) emitbyte(modrmv);
	if (sibv != -1) emitbyte(sibv);
	
	if (disppos && addrsize == X86_ADDRSIZE64) {
		// fix ip-relative disp in PM64 mode
		dispsize = 4;
		disp -= address + code.size + dispsize + immsize;
		if (simmsize(disp, 4) > 4) {
			clearcode();
			return false;
		}
	}
	switch (dispsize) {
	case 1:
		emitbyte(disp);
		break;
	case 2:
		emitword(disp);
		break;
	case 4:
		emitdword(disp);
		break;
	case 8:
		emitqword(disp);
		break;
	}
	switch (immsize) {
	case 1:
		emitbyte(imm);
		break;
	case 2:
		emitword(imm);
		break;
	case 4:
		emitdword(imm);
		break;
	case 6:
		emitdword(imm);
		emitword(imm2);
		break;
	case 8:
		emitqword(imm);
		break;
	}

	// fix rex code
	if (rexprefix & 0x40) {
		code.data[rexpos] = rexprefix;
	}
	
	return true;
}

bool x86asm::encode_modrm(x86_insn_op *op, char size, bool allow_reg, bool allow_mem, int eopsize, int eaddrsize)
{
	switch (op->type) {
	case X86_OPTYPE_REG:
		if (!allow_reg) return false;
		emitmodrm_mod(3);
		emitmodrm_rm(op->reg);
		if (op->reg > 7) rexprefix |= rexb;
		break;
	case X86_OPTYPE_MEM: {
		if (!allow_mem) return false;

		int mindispsize = op->mem.disp ? simmsize(op->mem.disp, 4) : 0;

		int addrsize = op->mem.addrsize;
		if (addrsize == X86_ADDRSIZEUNKNOWN) {
			addrsize=eaddrsize;
		} else {
			if (mindispsize > 4) return false;
		}
		if (addrsize == X86_ADDRSIZE16) {
			int mod, rm, dispsize;
			if (!encode_modrm_v(&modrm16, op, mindispsize, &mod, &rm, &dispsize)) return 0;
			emitmodrm_mod(mod);
			emitmodrm_rm(rm);
			emitdisp(op->mem.disp, dispsize);
		} else {
			int mod, rm, dispsize;
			if (!encode_modrm_v(&modrm32, op, mindispsize, &mod, &rm, &dispsize)) {
				int scale, index, base, disp=op->mem.disp;
				if (encode_sib_v(op, mindispsize, &scale, &index, &base, &mod, &dispsize, &disp)) {
					emitmodrm_mod(mod);
					emitmodrm_rm(4);			/* SIB */
					emitsib_scale(scale);
					emitsib_index(index);
					emitsib_base(base);
					emitdisp(disp, dispsize);
				} else return false;
			} else {
				emitmodrm_mod(mod);
				emitmodrm_rm(rm);
				if (this->addrsize == X86_ADDRSIZE64) disppos = 1;
				emitdisp(op->mem.disp, dispsize);
			}
		}
		break;
	}
	case X86_OPTYPE_MMX:
		if (!allow_reg) return false;
		emitmodrm_mod(3);
		emitmodrm_rm(op->mmx);
		break;
	case X86_OPTYPE_XMM:
		if (!allow_reg) return false;
		emitmodrm_mod(3);
		emitmodrm_rm(op->xmm);
		break;
	default:
		return false;
	}
	return true;
}

bool x86asm::encode_modrm_v(const x86addrcoding (*modrmc)[3][8], x86_insn_op *op, int mindispsize, int *_mod, int *_rm, int *_dispsize)
{
	if (op->mem.scale > 1) return false;
	for (int mod=0; mod < 3; mod++) {
		for (int rm=0; rm < 8; rm++) {
			const x86addrcoding *c=&(*modrmc)[mod][rm];
			int r1=c->reg1, r2=c->reg2;
			if (r2 == op->mem.base) {
				int t = r1;
				r1 = r2;
				r2 = t;
			}
			if (r1==op->mem.base && r2==op->mem.index && c->dispsize>=mindispsize) {
				*_mod=mod;
				*_rm=rm;
				*_dispsize=c->dispsize;
				return true;
			}
		}
	}
	return false;
}

bool x86asm::encode_op(x86_insn_op *op, x86opc_insn_op *xop, int *esize, int eopsize, int eaddrsize)
{
	int psize = op->size;
	switch (xop->type) {
	case TYPE_0:
		return true;
	case TYPE_A:
		/* direct address without ModR/M */
		if (op->type == X86_OPTYPE_FARPTR) {
			int size = esizeop_ex(xop->size, eopsize);
			emitfarptr(op->farptr.seg, op->farptr.offset, size == 6);
		} else {
			emitimm(op->imm, op->size);
		}
		break;
	case TYPE_C:
		/* reg of ModR/M picks control register */
		emitmodrm_reg(op->crx);
		if (op->crx > 7) rexprefix |= rexr;
		break;
	case TYPE_D:
		/* reg of ModR/M picks debug register */
		emitmodrm_reg(op->drx);
		if (op->drx > 7) rexprefix |= rexr;
		break;
	case TYPE_E:
		/* ModR/M (general reg or memory) */
		if (!encode_modrm(op, xop->size, true, true, eopsize, eaddrsize)) return false; //XXX
		psize = esizeop(xop->size, eopsize); //XXX
		break;
	case TYPE_F:
		/* r/m of ModR/M picks a fpu register */
		emitmodrm_rm(op->stx);
		break;
	case TYPE_Fx:
		/* extra picks a fpu register */
		return true;
	case TYPE_G:
		/* reg of ModR/M picks general register */
		emitmodrm_reg(op->reg);
		if (op->reg > 7) rexprefix |= rexr;		
		break;
	case TYPE_Is: {
		/* signed immediate */
		int size = esizeop_ex(xop->size, eopsize);
		emitimm(op->imm, size);
		break;
	}
	case TYPE_I: {
		/* unsigned immediate */
		int size = esizeop_ex(xop->size, eopsize);
		emitimm(op->imm, size);
		break;
	}
	case TYPE_Ix:
		/* fixed immediate */
		return true;
	case TYPE_J: {
		/* relative branch offset */
		int size = esizeop_ex(xop->size, eopsize);
		emitimm(uint32(op->imm - address - code.size - size), size);
		break;
	}
	case TYPE_M:
		/* ModR/M (memory only) */
		if (!encode_modrm(op, xop->size, false, true, eopsize, eaddrsize)) return false; // XXX
		psize = esizeop(xop->size, eopsize); //XXX
		break;
	case TYPE_O: {
		/* direct memory without ModR/M */
		if (op->mem.base != X86_REG_NO) return false;
		if (op->mem.index != X86_REG_NO) return false;
		psize = esizeop(xop->size, eopsize); // XXX
		switch (eaddrsize) {
		case X86_ADDRSIZE16:
			emitdisp(op->mem.disp, 2);
			break;
		case X86_ADDRSIZE32:
			emitdisp(op->mem.disp, 4);
			break;
		case X86_ADDRSIZE64:
			emitdisp(op->mem.disp, 8);
			break;
		}
		break;
	}
	case TYPE_P:
		/* reg of ModR/M picks MMX register */
		emitmodrm_reg(op->mmx);
		break;
	case TYPE_PR:
		/* rm of ModR/M picks MMX register */
		emitmodrm_mod(3);
		emitmodrm_rm(op->mmx);
		break;
	case TYPE_Q:
		/* ModR/M (MMX reg or memory) */
		if (!encode_modrm(op, xop->size, true, true, eopsize, eaddrsize)) return false; //XXX
		psize = esizeop(xop->size, eopsize); //XXX
		break;
	case TYPE_R:
		/* rm of ModR/M picks general register */
		emitmodrm_rm(op->reg);
		// fall throu
	case TYPE_Rx:
		if (op->reg > 7) rexprefix |= rexb;
		return true;
	case TYPE_RXx:
		/* extra picks register, no REX */
		return true;
	case TYPE_S:
		/* reg of ModR/M picks segment register */
		emitmodrm_reg(op->seg);
		break;
	case TYPE_Sx:
		/* extra picks segment register */
		return true;
	case TYPE_T:
		/* reg of ModR/M picks test register */
		emitmodrm_reg(op->trx);
		if (op->trx > 7) rexprefix |= rexr;
		break;
	case TYPE_VR:
		/* rm of ModR/M picks XMM register */
		emitmodrm_mod(3);
		// fall throu
	case TYPE_V:
		/* reg of ModR/M picks XMM register */
		emitmodrm_reg(op->xmm);
		if (op->mmx > 7) rexprefix |= rexr;
		break;
	case TYPE_W:
		/* ModR/M (XMM reg or memory) */
		if (!encode_modrm(op, xop->size, true, true, eopsize, eaddrsize)) return false; //XXX
		psize = esizeop(xop->size, eopsize); //XXX
		break;
	}
	if (!psize) {
//		set_error_msg(X86ASM_ERRMSG_INTERNAL"FIXME: size ??? %s, %d\n", __FILE__, __LINE__);
	}
	if (!*esize) *esize = psize;
/*	if (!(options & X86ASM_ALLOW_AMBIGUOUS) && *esize != psize) {
		ambiguous = 1;
		set_error_msg(X86ASM_ERRMSG_AMBIGUOUS);
		return 0;
	}*/
	return true;
}

bool x86asm::encode_sib_v(x86_insn_op *op, int mindispsize, int *_ss, int *_index, int *_base, int *_mod, int *_dispsize, int *disp)
{
	int ss, scale=op->mem.scale, index=op->mem.index, base=op->mem.base, mod, dispsize;
	if (base == X86_REG_NO && index != X86_REG_NO) {
		switch (scale) {
		case 1: case 4: case 8:
			break;
		case 2: case 3: case 5: case 9:
			scale--;
			base = index;
			break;
		default:
			return false;
		}
	}
	if ((index & 0x7) == X86_REG_SP) {
		if (scale > 1) return false;
		if (scale == 1) {
			if ((base & 0x7) == X86_REG_SP) return false;
			int temp = index;
			index = base;
			base = temp;
		}
	}
	if (index != X86_REG_NO) {
		switch (scale) {
		case 1:
			ss = 0;
			break;
		case 2:
			ss = 1;
			break;
		case 4:
			ss = 2;
			break;
		case 8:
			ss = 3;
			break;
		default:
			return 0;
		}				
	} else {
		ss = 0;
		index = 4;
	}		
	switch (mindispsize) {
	case 0:
		mod = 0;
		dispsize = 0;
		break;
	case 1:
		mod = 1;
		dispsize = 1;
		break;
	case 2:
	case 4:
	case 8:
		mod = 2;
		dispsize = 4;
		break;
	default:
		return false;
	}
	if (base == X86_REG_BP && mod == 0) {
		mod = 1;
		dispsize = 1;
		if (!mindispsize) *disp = 0;
	}
	if (base == X86_REG_NO) {
		base = 5;
		mod = 0;
		dispsize = 4;
		if (!mindispsize) *disp = 0;
		if (addrsize == X86_ADDRSIZE64) {
			disppos = 1;
		}
	}
	*_mod = mod;
	*_ss = ss;
	*_index = index;
	*_base = base;
	if (index > 7) rexprefix |= rexx;
	if (base > 7) rexprefix |= rexb;
	*_dispsize = dispsize;
	return 1;
}

int x86asm::esizeop(uint c, int size)
{
	switch (c) {
	case SIZE_B:
		return 1;
	case SIZE_W:
		return 2;
	case SIZE_D:
	case SIZE_S:
		return 4;
	case SIZE_Q:
	case SIZE_L:
		return 8;
	case SIZE_O:
		return 16;
	case SIZE_T:
		return 10;
	case SIZE_V:
	case SIZE_BV:
	case SIZE_VV:
		switch (size) {
		case X86_OPSIZE16: return 2;
		case X86_OPSIZE32: return 4;
		case X86_OPSIZE64: return 8;
		}
/*	case SIZE_R:
		if (rexw(insn.rexprefix)) return 8; else return 4;
	case SIZE_U:
		if (insn.opsizeprefix == X86_PREFIX_OPSIZE) return 16; else return 8;
	case SIZE_Z:
		if (insn.opsizeprefix == X86_PREFIX_OPSIZE) return 8; else return 4;
*/
	case SIZE_P:
		if (size == X86_OPSIZE16) return 4; else return 6;
	}
	return 0;
}

int x86asm::esizeop_ex(uint c, int size)
{
	switch (c) {
	case SIZE_BV:
		return 1;
	case SIZE_VV:
		switch (size) {
		case X86_OPSIZE16: return 2;
		case X86_OPSIZE32:
		case X86_OPSIZE64: return 4;
		}
	}
	return esizeop(c, size);
}


char x86asm::flsz2hsz(int size)
{
	switch (size) {
	case 4:
		return SIZE_S;
	case 8:
		return SIZE_L;
	case 10:
		return SIZE_T;
	}
	return 0;
}

const char *x86asm::get_name()
{
	return "x86asm";
}

const char *x86asm::immlsz2hsz(int size, int opsize)
{
	if (opsize == X86_OPSIZE16) {
		switch (size) {
		case 1:
			return immhsz8_16;
		case 2:
			return immhsz16_16;
		case 4:
			return immhsz32_16;
		case 8:
			return immhsz64_16;
		}
	} else if (opsize == X86_OPSIZE32) {
		switch (size) {
		case 1:
			return immhsz8_32;
		case 2:
			return immhsz16_32;
		case 4:
			return immhsz32_32;
		case 8:
			return immhsz64_32;
		}
	} else {
		switch (size) {
		case 1:
			return immhsz8_64;
		case 2:
			return immhsz16_64;
		case 4:
			return immhsz32_64;
		case 8:
			return immhsz64_64;
		}
	}
	return 0;
}

const char *x86asm::lsz2hsz(int size, int opsize)
{
	if (opsize == X86_OPSIZE16) {
		switch (size) {
		case 1:
			return hsz8_16;
		case 2:
			return hsz16_16;
		case 4:
			return hsz32_16;
		case 6:
			return hsz48_16;
		case 8:
			return hsz64_16;
		case 16:
			return hsz128_16;
		}
	} else if (opsize == X86_OPSIZE32) {
		switch (size) {
		case 1:
			return hsz8_32;
		case 2:
			return hsz16_32;
		case 4:
			return hsz32_32;
		case 6:
			return hsz48_32;
		case 8:
			return hsz64_32;
		case 16:
			return hsz128_32;
		}
	} else {
		switch (size) {
		case 1:
			return hsz8_64;
		case 2:
			return hsz16_64;
		case 4:
			return hsz32_64;
		case 6:
			return hsz48_64;
		case 8:
			return hsz64_64;
		case 16:
			return hsz128_64;
		}
	}
	return 0;
}

#define MATCHTYPE_NOMATCH	0
#define MATCHTYPE_MATCH		1
#define MATCHTYPE_NOOPPREFIX	2
#define MATCHTYPE_OPPREFIX	3

int x86asm::match_type(x86_insn_op *op, x86opc_insn_op *xop, int addrsize)
{
	const int *hop=lop2hop[op->type];
	if (op->type == X86_OPTYPE_EMPTY && xop->type == TYPE_0) return MATCHTYPE_MATCH;
	int r = MATCHTYPE_MATCH;
	if (xop->type == TYPE_W && xop->extra == 1) {
		r = MATCHTYPE_OPPREFIX;
	} else if (op->type == X86_OPTYPE_MMX) {
		if ((xop->type == TYPE_P || xop->type == TYPE_PR || xop->type == TYPE_Q)
		&& (xop->size == SIZE_U || xop->size == SIZE_Z)) {
			r = MATCHTYPE_NOOPPREFIX;
		}
	}
	while (*hop) {
		if (*hop == xop->type) {
			if (xop->type == TYPE_Rx) {
				if (xop->extra == (op->reg & 7)) return r;
			} else if (xop->type == TYPE_RXx) {
				if (xop->extra == op->reg) return r;
			} else if (xop->type == TYPE_Sx) {
				if (xop->extra == op->seg) return r;
			} else if (xop->type == TYPE_Ix) {
				if ((unsigned)xop->extra == op->imm) return r;
			} else if (xop->type == TYPE_Fx) {
				if (xop->extra == op->stx) return r;
			} else if (op->type == X86_OPTYPE_MEM) {
				if (op->mem.addrsize == addrsize
				 || op->mem.addrsize == X86_ADDRSIZEUNKNOWN) return r;
			} else return r;
		}
		hop++;
	}
	// special xmm match of mmx operands
	if (op->type == X86_OPTYPE_XMM) {
		if ((xop->type == TYPE_P || xop->type == TYPE_PR || xop->type == TYPE_Q)
		&& (xop->size == SIZE_U || xop->size == SIZE_Z)) {
			return MATCHTYPE_OPPREFIX;
		}
	}
	return MATCHTYPE_NOMATCH;
}

bool x86asm::match_size(x86_insn_op *op, x86opc_insn_op *xop, int opsize)
{
	if (op->type == X86_OPTYPE_EMPTY && xop->type == TYPE_0) return true;
	if (!op->size && xop->type != TYPE_0) return true;
	const char *hsz = NULL;
	if ((op->type == X86_OPTYPE_MEM && op->mem.floatptr)
	 ||  op->type == X86_OPTYPE_STX) {
		return xop->size == flsz2hsz(op->size);
	} else if (op->type == X86_OPTYPE_IMM) {
		if (xop->type == TYPE_Is) {
			hsz = immlsz2hsz(simmsize(op->imm, esizeop(xop->size, opsize)), opsize); //XXX
		} else if (xop->type == TYPE_J) {
			int size = esizeop(xop->size, opsize);
			// FIXME: ?!
			hsz = immlsz2hsz(simmsize(uint32(op->imm - address - code.size - size - 1), 4), opsize);
		} else {
			hsz = immlsz2hsz(simmsize(op->imm, esizeop(xop->size, opsize)), opsize); //XXX
//			hsz = immlsz2hsz(op->size, opsize);
		}
	} else if (op->type == X86_OPTYPE_MMX) {
		return true;
	} else if (op->type == X86_OPTYPE_XMM) {
		return true;
	} else {
		hsz = lsz2hsz(op->size, opsize);
	}
	if (hsz) {
		while (*hsz) {
			if (*hsz == xop->size) return true;
			hsz++;
		}
	}
	return false;
}

int x86asm::match_allops(x86asm_insn *insn, x86opc_insn *xinsn, int opsize, int addrsize)
{
	int m = 0;
	for (int i = 0; i < 3; i++) {
		int m2 = match_type(&insn->op[i], &xinsn->op[i], addrsize);
		if (!m2 || (m && m != MATCHTYPE_MATCH && m2 != MATCHTYPE_MATCH && m != m2)) {
			return MATCHTYPE_NOMATCH;
		} else {
			if (m2 > m) m = m2;
		}
		if (!match_size(&insn->op[i], &xinsn->op[i], opsize)) return MATCHTYPE_NOMATCH;
	}
	return m;
}

#define MATCHOPNAME_NOMATCH		0
#define MATCHOPNAME_MATCH		1
#define MATCHOPNAME_MATCH_IF_OPSIZE16	2
#define MATCHOPNAME_MATCH_IF_OPSIZE32	3
#define MATCHOPNAME_MATCH_IF_OPSIZE64	4
#define MATCHOPNAME_MATCH_IF_ADDRSIZE16	5
#define MATCHOPNAME_MATCH_IF_ADDRSIZE32	6
#define MATCHOPNAME_MATCH_IF_ADDRSIZE64	7
#define MATCHOPNAME_MATCH_IF_OPPREFIX	8
#define MATCHOPNAME_MATCH_IF_NOOPPREFIX	9

static void pickname(char *result, const char *name, int n)
{
	const char *s = name;
	do {
		name = s+1;
		s = strchr(name, '|');
		if (!s) {
			strcpy(result, name);
			return;
		}
	} while (n--);
	ht_strlcpy(result, name, s-name+1);
}

int x86asm::match_opcode_name(const char *input_name, const char *opcodelist_name)
{
	if (opcodelist_name) {
		char n1[32], n2[32], n3[32];
		pickname(n1, opcodelist_name, 0);
		pickname(n2, opcodelist_name, 1);
		pickname(n3, opcodelist_name, 2);
		switch (opcodelist_name[0]) {
		case '|':
			if (strcmp(n1, input_name)==0) return MATCHOPNAME_MATCH;
			if (strcmp(n2, input_name)==0) return MATCHOPNAME_MATCH;
			if (strcmp(n3, input_name)==0) return MATCHOPNAME_MATCH;
			break;
		case '?':
			if (strcmp(n1, input_name)==0) return MATCHOPNAME_MATCH_IF_OPSIZE16;
			if (strcmp(n2, input_name)==0) return MATCHOPNAME_MATCH_IF_OPSIZE32;
			if (strcmp(n3, input_name)==0) return MATCHOPNAME_MATCH_IF_OPSIZE64;
			break;
		case '*':
			if (strcmp(n1, input_name)==0) return MATCHOPNAME_MATCH_IF_ADDRSIZE16;
			if (strcmp(n2, input_name)==0) return MATCHOPNAME_MATCH_IF_ADDRSIZE32;
			if (strcmp(n3, input_name)==0) return MATCHOPNAME_MATCH_IF_ADDRSIZE64;
			break;
		case '&':
			if (strcmp(n1, input_name)==0) return MATCHOPNAME_MATCH_IF_NOOPPREFIX;
			if (strcmp(n2, input_name)==0) return MATCHOPNAME_MATCH_IF_OPPREFIX;
			break;
		default:
			if (strcmp(opcodelist_name, input_name)==0) return MATCHOPNAME_MATCH;
		}
	}
	return MATCHOPNAME_NOMATCH;
}	

#define OPSIZE_INV(opsize) (opsize==X86_OPSIZE16 ? X86_OPSIZE32 : X86_OPSIZE16)
#define ADDRSIZE_INV(addrsize) (addrsize==X86_ADDRSIZE16 ? X86_ADDRSIZE32 : X86_ADDRSIZE16)

static void swap(char &a, char &b)
{
	char tmp = a;
	a = b; b = tmp;
}

void x86asm::match_opcode(x86opc_insn *opcode, x86asm_insn *insn, int prefix, byte opcodebyte, int additional_opcode)
{
	int n = match_opcode_name(insn->name, opcode->name);
	namefound |= n;
	if (n != MATCHOPNAME_NOMATCH) {
		insn->opsizeprefix = X86_PREFIX_NO;
		char opsizes[] = {X86_OPSIZE16, X86_OPSIZE32, X86_OPSIZE64};
		char addrsizes[] = {X86_ADDRSIZE16, X86_ADDRSIZE32, X86_ADDRSIZE64};
		
		switch (addrsize) {
		case X86_ADDRSIZE32: swap(addrsizes[0], addrsizes[1]); break;
		case X86_ADDRSIZE64: swap(addrsizes[0], addrsizes[2]); break;
		}
		
		bool done1 = false;
		/*
		 * check all permutations of opsize and addrsize
		 * if possible and necessary
		 */
		for (int o=0; o < 3; o++) {
			switch (n) {
			case MATCHOPNAME_MATCH_IF_OPSIZE16: done1 = true; break;
			case MATCHOPNAME_MATCH_IF_OPSIZE32: o = 1; done1 = true; break;
			case MATCHOPNAME_MATCH_IF_OPSIZE64: o = 2; done1 = true; break;
			}
			if (o == 2 && addrsize != X86_ADDRSIZE64) break;
			bool done2 = !addrsize_depend;
			for (int a=0; a < 2; a++) {
				char as = addrsizes[a];
		    		switch (n) {
				case MATCHOPNAME_MATCH_IF_ADDRSIZE16: as = X86_ADDRSIZE16; done2 = true; break;
				case MATCHOPNAME_MATCH_IF_ADDRSIZE32: as = X86_ADDRSIZE32; done2 = true; break;
		    		case MATCHOPNAME_MATCH_IF_ADDRSIZE64: as = X86_ADDRSIZE64; done2 = true; break;
				}
				match_opcode_final(opcode, insn, prefix, opcodebyte, additional_opcode, opsizes[o], as, n);
				if (done2) break;
			}
			if (done1) break;
		}
	}
}

int x86asm::match_opcode_final(x86opc_insn *opcode, x86asm_insn *insn, int prefix, byte opcodebyte, int additional_opcode, int opsize, int addrsize, int match)
{
	switch (match_allops(insn, opcode, opsize, addrsize)) {
	case MATCHTYPE_NOMATCH:
		return false;
	case MATCHTYPE_MATCH:
		if (match == MATCHOPNAME_MATCH_IF_OPPREFIX) {
			insn->opsizeprefix = X86_PREFIX_OPSIZE;
		}
		break;
	case MATCHTYPE_NOOPPREFIX:
		if (match == MATCHOPNAME_MATCH_IF_OPPREFIX) return false;
		break;
	case MATCHTYPE_OPPREFIX:
		if (match == MATCHOPNAME_MATCH_IF_NOOPPREFIX) return false;
		insn->opsizeprefix = X86_PREFIX_OPSIZE;
		break;
	}
//	printf("o%ds%d: %02x (%d)\n", (opsize==X86_OPSIZE16) ? 16 : 32, (addrsize==X86_ADDRSIZE16) ? 16 : 32, opcodebyte, opcodebyte);
	if (encode_insn(insn, opcode, opcodebyte, additional_opcode, prefix, opsize, addrsize)) {
		pushcode();
		newcode();
	}
	return true;
}

void x86asm::match_opcodes(x86opc_insn *opcodes, x86asm_insn *insn, int prefix)
{
	for (int i=0; i < 256; i++) {
		if (!opcodes[i].name) {
			x86opc_insn_op_special *special = ((x86opc_insn_op_special*)(&opcodes[i].op[0]));
			if (special->type == SPECIAL_TYPE_GROUP) {
				x86opc_insn *group = x86_32_group_insns[special->data];
				for (int g=0; g < 8; g++) {
					if (!group[g].name) {
						x86opc_insn_op_special *special2 = ((x86opc_insn_op_special*)(&group[g].op[0]));
						if (special2->type == SPECIAL_TYPE_SGROUP) {
							x86opc_insn *group = x86_special_group_insns[special2->data];
							for (int h=0; h < 8; h++) {
								match_opcode(&group[h], insn, prefix, i, (h<<3) + g + 0x800);
							}
							match_opcode(&group[8], insn, prefix, i, -1);
						}
					} else {
						match_opcode(&group[g], insn, prefix, i, g);
					}
				}
			}
		} else {
			match_opcode(&opcodes[i], insn, prefix, i, -1);
		}
	}
}

void x86asm::match_fopcodes(x86asm_insn *insn)
{
	/* try modrm fopcodes */
	for (int i=0; i < 8; i++) {
		for (int j=0; j < 8; j++) {
			int n = match_opcode_name(insn->name, x86_modfloat_group_insns[i][j].name);
			namefound |= n;
			if (n != MATCHOPNAME_NOMATCH) {
				int eaddrsize = addrsize;
				for (int k=0; k < 2; k++) {
					if (match_allops(insn, &x86_modfloat_group_insns[i][j], opsize, eaddrsize)) {
						if (encode_insn(insn, &x86_modfloat_group_insns[i][j], j<<3, -1, X86ASM_PREFIX_D8+i, opsize, eaddrsize)) {
							pushcode();
							newcode();
						}
					}
					switch (eaddrsize) {
					case X86_ADDRSIZE64:
					case X86_ADDRSIZE16: eaddrsize = X86_ADDRSIZE32; break;
					case X86_ADDRSIZE32: eaddrsize = X86_ADDRSIZE16; break;
					}
				}
			}
		}
	}
	/* try the rest */
	for (int i=0; i<8; i++) {
		for (int j=0; j<8; j++) {
			if (x86_float_group_insns[i][j].group==0) {
				int n=match_opcode_name(insn->name, x86_float_group_insns[i][j].insn.name);
				namefound |= n;
				if (n != MATCHOPNAME_NOMATCH) {
					if (match_allops(insn, &x86_float_group_insns[i][j].insn, opsize, addrsize)) {
						if (encode_insn(insn, &x86_float_group_insns[i][j].insn, 0xc0 | j<<3, -1, X86ASM_PREFIX_D8+i, opsize, addrsize)) {
							pushcode();
							newcode();
						}
						if (error) return;
					}
				}
			} else {
				x86opc_insn *group=x86_float_group_insns[i][j].group;
				for (int k=0; k<8; k++) {
					int n = match_opcode_name(insn->name, group[k].name);
					namefound |= n;
					if (n != MATCHOPNAME_NOMATCH) {
						int eaddrsize = addrsize;
						for (int l=0; l < 2; l++) {
							if (match_allops(insn, &group[k], opsize, eaddrsize)) {
								if (encode_insn(insn, &group[k], 0xc0 | j<<3 | k, -1, X86ASM_PREFIX_D8+i, opsize, eaddrsize)) {
									pushcode();
									newcode();
								}
							}
							switch (eaddrsize) {
							case X86_ADDRSIZE64:
							case X86_ADDRSIZE16: eaddrsize = X86_ADDRSIZE32; break;
							case X86_ADDRSIZE32: eaddrsize = X86_ADDRSIZE16; break;
							}
						}
					}
				}
			}
		}
	}
}

bool x86asm::opreg(x86_insn_op *op, const char *xop)
{
	for (int i=0; i<3; i++) {
		for (int j=0; j<8; j++) {
			if (x86_regs[i][j] && strcmp(xop, x86_regs[i][j])==0) {
				op->type = X86_OPTYPE_REG;
				op->size = reg2size[i];
				op->reg = j;
				return true;
			}
		}
	}
	return false;
}

bool x86asm::opmmx(x86_insn_op *op, const char *xop)
{
	if (strlen(xop) == 3 && xop[0] == 'm' && xop[1] == 'm'
	 && xop[2] >= '0' && xop[2] <= '7') {
		op->type = X86_OPTYPE_MMX;
		op->size = 8;
		op->mmx = xop[2] - '0';
		return true;
	} else {
		return false;
	}
}

bool x86asm::opxmm(x86_insn_op *op, const char *xop)
{
	if (strlen(xop) == 4 && xop[0] == 'x' && xop[1] == 'm' && xop[2] == 'm'
	 && xop[3] >= '0' && xop[3] <= '7') {
		op->type = X86_OPTYPE_XMM;
		op->size = 16;
		op->xmm = xop[3] - '0';
		return true;
	} else {
		return false;
	}
}

bool x86asm::opseg(x86_insn_op *op, const char *xop)
{
	for (int i=0; i<8; i++) {
		if (x86_segs[i] && strcmp(xop, x86_segs[i])==0) {
			op->type = X86_OPTYPE_SEG;
			op->size = 2;
			op->seg = i;
			return true;
		}
	}
	return false;
}

bool x86asm::opfarptr(x86_insn_op *op, const char *xop)
{
	return false;
/*
FIXME:
	uint64 seg, offset;
	char *x = xop;
	
	if (!fetch_number(&x, &seg)) return false;
	if (*x != ':') return false;
	x++;
	if (!fetch_number(&x, &offset)) return false;
	if (*x) return false;
	op->type = X86_OPTYPE_FARPTR;
	if (offset > 0xffff) op->size=6; else op->size=4;
	op->farptr.seg = seg;
	op->farptr.offset = offset;
	return true;
*/
}

bool x86asm::opimm(x86_insn_op *op, const char *xop)
{
	uint64 i;
	if (!str2int(xop, i)) return false;
	op->type = X86_OPTYPE_IMM;
	if (i > 0xffffffffULL) {
		op->size = 8; 
	} else if (i > 0xffff) {
		op->size = 4; 
	} else if (i > 0xff) {
		op->size = 2; 
	} else {
		op->size = 1;
	}
	op->imm = i;
	return true;
}

bool x86asm::opplugimm(x86_insn_op *op, const char *xop)
{
	uint64 d;
	if (imm_eval_proc && imm_eval_proc(imm_eval_context, xop, d)) {
		op->type = X86_OPTYPE_IMM;
		if (d > 0xffffffff) {
			op->size = 8; 
		} else if (d > 0xffff) {
			op->size = 4; 
		} else if (d > 0xff) {
			op->size = 2; 
		} else {
			op->size = 1;
		}
		op->imm = d;
		return true;
	}
	return false;
}

bool x86asm::opmem(x86asm_insn *asm_insn, x86_insn_op *op, const char *s)
{
	/* FIXME: dirty implementation! */
	int opsize = 0, hsize = 0;
	bool floatptr = false;
	char token[256];
	const char *sep = "[]()*+-:";

	tok(&s, token, sizeof token, sep);
	
	static const char *types[] = {"byte", "word", "dword", "pword", "qword", "oword", "single", "double", "extended"};
	static byte type_size[] = {1, 2, 4, 6, 8, 16, 4, 8, 10};
	// typecast
	for (uint i=0; i < sizeof types / sizeof types[0]; i++) {
		if (strcmp(token, types[i]) == 0) {
			hsize = type_size[i];
			if (i >= 6) floatptr = true;
			break;
		}
	}
	if (hsize) {
		tok(&s, token, sizeof token, sep);		
		if (!(strcmp(token, "ptr") == 0)) return false;
		opsize = hsize;
		tok(&s, token, sizeof token, sep);
	}

	// segprefixes (e.g. fs:)
	for (int i = 0; i < 8; i++) {
		if (x86_segs[i]) {
			if (strcmp(x86_segs[i], token)==0) {
				tok(&s, token, sizeof token, sep);				
				if (!(strcmp(token, ":") == 0)) return false;
				static const int c2p[8] = {X86_PREFIX_ES, X86_PREFIX_CS, X86_PREFIX_SS, X86_PREFIX_DS, X86_PREFIX_FS, X86_PREFIX_GS, 0, 0};
				asm_insn->segprefix = c2p[i];
				tok(&s, token, sizeof token, sep);				
				break;
			}
		}
	}

	if (!(strcmp(token, "[") == 0)) return false;

	int scale = 0, index = X86_REG_NO, base = X86_REG_NO;
	uint64 disp = 0;
	int addrsize = X86_ADDRSIZEUNKNOWN;
	int lasttokenreg = X86_REG_NO;
	bool need_rex = false;

	int sign = 1;
	while (1) {
cont:
		tok(&s, token, sizeof token, sep);
		if (strcmp(token, "+") == 0) {
			if (!sign) sign = 1;
			continue;
		}
		if (strcmp(token, "-") == 0) {
			if (sign) {
				sign = -sign;
			} else {
				sign = -1;
			}
			continue;
		}
		if (strcmp(token, "]") == 0) {
			if (sign) return false;
			break;
		}
		if (strcmp(token, "*") == 0) {
			tok(&s, token, sizeof token, sep);
			if (lasttokenreg == X86_REG_NO) {
				/* FIXME: case "imm*reg" not yet supported! 
				cleaner implementation needed! */
				return false;
			} else {
				uint64 v;
				if (!str2int(token, v)) return false;
				if (v > 1) {
					if (index == lasttokenreg) {
						scale += v-1;
					} else if (base == lasttokenreg) {
						if (index != X86_REG_NO) return false;
						index = base;
						base = X86_REG_NO;
						scale = v;
					}
				}
			}
			lasttokenreg = X86_REG_NO;
			sign = 0;
			continue;
		}
		/* test if reg */
		for (int i=1; i < 4; i++) {
			for (int j=0; j < 16; j++) {
				if (x86_64regs[i][j] && strcmp(token, x86_64regs[i][j])==0) {
					if (j > 7 || i == 3) {
						if (this->addrsize != X86_ADDRSIZE64) break;
						if (j > 7) need_rex = true;
					}
					if (sign < 0) return false;
					static const byte sizer[] = {X86_ADDRSIZE16, X86_ADDRSIZE32, X86_ADDRSIZE64};
					int caddrsize = sizer[i-1];
					if (addrsize == X86_ADDRSIZEUNKNOWN) {
						addrsize = caddrsize;
					} else if (addrsize != caddrsize) {
						return false;
					}
					if (index == j) {
						scale++;
					} else if (base == X86_REG_NO) {
						base = j;
					} else if (index == X86_REG_NO) {
						index = j;
						scale = 1;
					} else if (base == j && scale == 1) {
						int t = index;
						index = base;
						base = t;
						scale = 2;
					} else return false;
					lasttokenreg = j;
					sign = 0;
					goto cont;
				}
			}
		}
		lasttokenreg = X86_REG_NO;

		/* test if number */
		uint64 v;
		if ((imm_eval_proc && imm_eval_proc(imm_eval_context, token, v))
		 || str2int(token, v)) {
			if (!sign) return false;
			if (sign < 0) disp -= v; else disp += v;
			sign = 0;
			continue;
		}
		return false;
	}

	if (base == X86_REG_NO && index == X86_REG_NO) {
		/* unsigned disp */
		if (addrsize != X86_ADDRSIZE64) {
			if (disp > 0xffffffffUL) return false;
			if (disp > 0xffff) {
				if (addrsize == X86_ADDRSIZEUNKNOWN) {
					addrsize = X86_ADDRSIZE32;
				} else if (addrsize != X86_ADDRSIZE32) return false;
			}
		}
	} else {
		/* signed disp */
		if (addrsize != X86_ADDRSIZE64) {
			int s = simmsize(disp, 4);
			if (s > 4) return false;
			if (s > 2) {
				if (addrsize == X86_ADDRSIZEUNKNOWN) {
					addrsize = X86_ADDRSIZE32;
				} else if (addrsize != X86_ADDRSIZE32) return false;
			}
		}		
	}
	op->type = X86_OPTYPE_MEM;
	op->size = opsize;
	op->mem.base = base;
	op->mem.index = index;
	op->mem.scale = scale;
	op->mem.addrsize = addrsize;
	op->mem.disp = disp;
	op->mem.floatptr = floatptr;
	op->need_rex = need_rex;
//	int r = (addrsize==X86_ADDRSIZE16) ? 1 : 2;
//	printf("%s.%d: opmem(): size=%d, base = %s, index = %s, scale = %d, disp=%08lx, addrsize = %d\n", __FILE__, __LINE__, opsize, (base==X86_REG_NO) ? "" : x86_regs[r][base], (index==X86_REG_NO) ? "" : x86_regs[r][index], scale, disp, addrsize);
	return true;
}

bool x86asm::opspecialregs(x86_insn_op *op, const char *xop)
{
	char *e;
	if (strcmp(xop, "st")==0) {
		op->type=X86_OPTYPE_STX;
		op->size=10;
		op->stx=0;
		return true;
	} else if (ht_strncmp(xop, "st", 2)==0 && xop[2]=='(' && xop[4]==')') {
		int w = strtol(xop+3, &e, 10);
		if (e != xop+4 || w > 7) return false;
		op->type = X86_OPTYPE_STX;
		op->size = 10;
		op->stx = w;
		return 1;
	}

	/* FIXME: do we need this? 
	 * strtol sets e to next untranslatable char, 
	 * this case is caught below... 
	 */
	if (strlen(xop) != 3) return 0;

	int w = strtol(xop+2, &e, 10);
	if (*e || w > 7) return 0;
	if (ht_strncmp(xop, "cr", 2) == 0) {
		op->type = X86_OPTYPE_CRX;
		op->size = 4;
		op->crx = w;
		return true;
	} else if (ht_strncmp(xop, "dr", 2) == 0) {
		op->type = X86_OPTYPE_DRX;
		op->size = 4;
		op->drx = w;
		return true;
	} else if (ht_strncmp(xop, "tr", 2) == 0) {
		op->type = X86_OPTYPE_TRX;
		op->size = 4;
		op->trx = w;
		return true;
	}
	return false;
}

bool x86asm::translate_str(asm_insn *asm_insn, const char *s)
{
	x86asm_insn *insn=(x86asm_insn*)asm_insn;
	char *opp[3], op[3][256];
	opp[0]=op[0];
	opp[1]=op[1];
	opp[2]=op[2];
	for (int i=0; i<3; i++) {
		insn->op[i].need_rex = insn->op[i].forbid_rex = false;
		insn->op[i].type = X86_OPTYPE_EMPTY;
	}

	insn->lockprefix = X86_PREFIX_NO;
	insn->repprefix = X86_PREFIX_NO;
	insn->segprefix = X86_PREFIX_NO;
	insn->opsizeprefix = X86_PREFIX_NO;

	const char *p = s, *a, *b;

	/* prefixes */
	whitespaces(p);
	a=p;
	non_whitespaces(p);
	b=p;
	if (ht_strncmp(a, "rep", b-a) == 0 || ht_strncmp(a, "repe", b-a) == 0
	 || ht_strncmp(a, "repz", b-a) == 0) {
		insn->repprefix=X86_PREFIX_REPZ;
		s = p;
	} else if (ht_strncmp(a, "repne", b-a) == 0 || ht_strncmp(a, "repnz", b-a) == 0) {
		insn->repprefix=X86_PREFIX_REPNZ;
		s = p;
	} else if (ht_strncmp(a, "lock", b-a) == 0) {
		insn->lockprefix=X86_PREFIX_LOCK;
		s = p;
	}

	/**/
	splitstr(s, insn->n, sizeof insn->n, (char**)&opp, 256);
	insn->name=insn->n;
	for (int i=0; i<3; i++) {
		if (!*op[i]) break;

		if (!(opplugimm(&insn->op[i], op[i])
		 || opreg(&insn->op[i], op[i])
		 || opmmx(&insn->op[i], op[i])
		 || opxmm(&insn->op[i], op[i])
		 || opfarptr(&insn->op[i], op[i])
		 || opimm(&insn->op[i], op[i])
		 || opseg(&insn->op[i], op[i])
		 || opmem(insn, &insn->op[i], op[i])
		 || opspecialregs(&insn->op[i], op[i]))) {
			set_error_msg(X86ASM_ERRMSG_UNKNOWN_SYMBOL, op[i]);
			return false;
		}
	}
	return true;
}

int x86asm::simmsize(uint64 imm, int immsize)
{
	switch (immsize) {
	case 1:
		if (imm <= 0xff) return 1;
		break;
	case 2:
		if (imm <= 0xffff) imm = sint64(sint16(imm));
		break;
	case 4:
		if (imm <= 0xffffffff) imm = sint64(sint32(imm));
		break;
	}
	if (imm >= 0xffffffffffffff80ULL || imm < 0x80) return 1;
	if (imm >= 0xffffffffffff8000ULL || imm < 0x8000) return 2;
	if (imm >= 0xffffffff80000000ULL || imm < 0x80000000) return 4;
	return 8;
}

void x86asm::splitstr(const char *s, char *name, int size, char *op[3], int opsize)
{
	const char *a, *b;
	bool wantbreak = false;
	*name=0;
	*op[0]=0;
	*op[1]=0;
	*op[2]=0;
	/* find name */
	whitespaces(s);
	a = s;
	non_whitespaces(s);
	b = s;
	ht_strlcpy(name, a, MIN(b-a+1, size));
	/* find ops */
	for (int i = 0; i < 3; i++) {
		whitespaces(s);
		if (!*s) break;
		a = s;
		waitforchar(s, ',');
		while (is_whitespace(s[-1])) s--;
		if (!*s) wantbreak = true;
		b = s;
		whitespaces(s);
		if (!*s) wantbreak = true;
		ht_strlcpy(op[i], a, MIN(b-a+1, opsize));
		whitespaces(s);
		if (wantbreak || *s != ',') break;
		s++;
	}
}

void x86asm::tok(const char **s, char *res, int reslen, const char *sep)
{
	if (reslen <= 0) return;
	whitespaces(*s);
	if (strchr(sep, **s)) {
		if (reslen > 0) *res++ = *((*s)++);
	} else {
		while (reslen > 1) {
			*res++ = *((*s)++);
			reslen--;
			if (**s == ' ' || **s == '\t') break;
			if (strchr(sep, **s)) break;
		}
	}
	*res = 0;
}

/************************************************************************
 *
 */
x86opc_insn (*x86_64asm::x86_64_insns)[256];

x86_64asm::x86_64asm()
	: x86asm(X86_OPSIZE32, X86_ADDRSIZE64)
{
	prepInsns();
}

void x86_64asm::prepInsns()
{
	if (!x86_64_insns) {
		x86_64_insns = ht_malloc(sizeof *x86_64_insns);
		memcpy(x86_64_insns, x86_32_insns, sizeof x86_32_insns);
	
		int i = 0;
		while (x86_64_insn_patches[i].opc != -1) {
			(*x86_64_insns)[x86_64_insn_patches[i].opc] = x86_64_insn_patches[i].insn;
			i++;
		}
	}
	x86_insns = x86_64_insns;
}

bool x86_64asm::opreg(x86_insn_op *op, const char *xop)
{
	for (int i=0; i < 4; i++) {
		for (int j=0; j < 16; j++) {
			if (x86_64regs[i][j] && strcmp(xop, x86_64regs[i][j])==0) {
				op->type = X86_OPTYPE_REG;
				op->size = reg2size[i];
				op->reg = j;
				if (j > 7 || i == 3 || (i == 0 && j > 3)) {
					op->need_rex = true;
				}
				return true;
			}
		}
	}
	// check for legacy ah, ch, dh, bh
	for (int j=4; j < 8; j++) {
		if (x86_regs[0][j] && strcmp(xop, x86_regs[0][j])==0) {
			op->type = X86_OPTYPE_REG;
			op->size = reg2size[0];
			op->reg = j;
			op->forbid_rex = true;
			return true;
		}
	}
	return false;
}

bool x86_64asm::opxmm(x86_insn_op *op, const char *xop)
{
	int slen = strlen(xop);
	if ((slen == 4 || slen == 5) && xop[0] == 'x' && xop[1] == 'm' && xop[2] == 'm'
	 && xop[3] >= '0' && xop[3] <= '9') {
		int x = xop[3] - '0';
		if (slen == 5) {
			if (xop[4] < '0' || xop[4] > '9') return false;
			x *= 10;
			x += xop[4] - '0';
			if (x > 15) return false;
		}
		op->type = X86_OPTYPE_XMM;
		op->size = 16;
		op->xmm = x;
		if (x > 7) op->need_rex = true;
		return true;
	} else {
		return false;
	}
}
