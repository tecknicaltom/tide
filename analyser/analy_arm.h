#ifndef ANALY_ARM_20060408_H
#define ANALY_ARM_20060408_H

#include "analy.h"

class AnalyArmDisassembler: public AnalyDisassembler
{
public:
    AnalyArmDisassembler() {}
    AnalyArmDisassembler(BuildCtorArg&);
                void                    init(Analyser *A);
    virtual	void			done();
    virtual	ObjectID		object_id() const;
    virtual	Address *		branchAddr(OPCODE *opcode, branch_enum_t branchtype, bool examine);
    virtual	void			examineOpcode(OPCODE *opcode);
    virtual	branch_enum_t		isBranch(OPCODE *opcode);
};

#endif
