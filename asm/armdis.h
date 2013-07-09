#ifndef ARMDIS_20060407_H
#define ARMDIS_20060407_H

#include "asm.h"

struct ArmDisInsn
{
        bool        invalid;
        bool        iscond;
	byte        size;
        unsigned    offset;
        char        opstr[100];
};

class ArmDisassembler : public Disassembler
{
        class BFDInterface;
        BFDInterface *bfdif;

protected:
        ArmDisInsn insn;

        virtual	dis_insn	*decode(const byte *code, int maxlen, CPU_ADDR addr);
        virtual	dis_insn	*duplicateInsn(const dis_insn *disasm_insn);
        virtual	void		getOpcodeMetrics(int &min_length, int &max_length, int &min_look_ahead, int &avg_look_ahead, int &addr_align);
        virtual	byte		getSize(const dis_insn *disasm_insn);
        virtual	const char	*getName() const;
        virtual	const char	*strf(const dis_insn *disasm_insn, int style, const char *format);
        virtual	ObjectID        getObjectID() const;
        virtual	bool		validInsn(const dis_insn *disasm_insn);

public:
	ArmDisassembler();
	ArmDisassembler(BuildCtorArg&a): Disassembler(a) {};

	void load(ObjectStream &f);
};

#endif
