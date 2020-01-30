
#ifndef XSC_I_CODE
#define XSC_I_CODE

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"
#include "func_table.h"
#include "../bytecode.h"

// ---- Constants -----------------------------------------------------------------------------

// ---- I-Code Node Types -----------------------------------------------------------------

#define ICODE_NODE_INSTR 0           // An I-code instruction
#define ICODE_NODE_ANNOTATION_LINE 1 // Source-code annotation
#define ICODE_NODE_JUMP_TARGET 2     // A jump target

// ---- I-Code Opcode 采用了和执行体（CRL）相同的指令集

// ---- I-Code Operand Types ---------------------------------------------------------------------

//#define OP_TYPE_INT					0           // Integer literal value
//#define OP_TYPE_FLOAT					1           // Floating-point literal value
//#define OP_TYPE_STRING_INDEX			2           // String literal value
//#define ICODE_OP_TYPE_VAR_NAME					3           // Variable
//#define OP_TYPE_ABS_STACK_INDEX				4           // Array with absolute index
//#define OP_TYPE_REL_STACK_INDEX				5           // Array with relative index
//#define ICODE_OP_TYPE_JUMP_TARGET			6           // Jump target index
//#define OP_TYPE_FUNC_INDEX			7           // Function index
////#define OP_TYPE_HOST_FUNC					8           // Host Function
//#define OP_TYPE_REG					9           // Register

typedef int Label;

// An I-code operand
struct Op
{
    int iType; // Type
    union      // The value
    {
        int iIntLiteral;     // Integer literal
        float fFloatLiteral; // Float literal
        int iStringIndex;    // String table index
        int iSymbolIndex;    // Symbol table index
        Label label;         // Jump target index
        int iFuncIndex;      // Function index
        int iRegCode;        // Register code
    };
    int iOffset;            // Immediate offset
    int iOffsetSymbolIndex; // Offset symbol index
};

struct ICodeInstr // An I-code instruction
{
    int iOpcode;       // Opcode
    LinkedList OpList; // Operand list
};

struct ICodeNode // An I-code node
{
    int iType; // The node type
    union {
        ICodeInstr Instr;       // The I-code instruction
        char *pstrSourceLine;   // The source line with which this instruction is annotated
        Label iJumpTargetIndex; // The jump target index
    };
};

// ---- Function Prototypes -------------------------------------------------------------------

ICodeNode *GetICodeNodeByImpIndex(int iFuncIndex, int iInstrIndex);
Op *GetICodeOpByIndex(ICodeNode *pInstr, int iOpIndex);

void AddICodeAnnotation(int iFuncIndex, char *pstrSourceLine);
int AddICodeInstr(int iFuncIndex, int iOpcode);
int AddICodeInstr(int iFuncIndex, int iOpcode, Label label);
void AddICodeOp(int iFuncIndex, int iInstrIndex, Op Value);
void AddIntICodeOp(int iFuncIndex, int iInstrIndex, int iValue);
void AddFloatICodeOp(int iFuncIndex, int iInstrIndex, float fValue);
void AddStringICodeOp(int iFuncIndex, int iInstrIndex, int iStringIndex);
void AddVarICodeOp(int iFuncIndex, int iInstrIndex, int iSymbolIndex);
void AddArrayIndexAbsICodeOp(int iFuncIndex, int iInstrIndex, int iArraySymbolIndex, int iOffset);
void AddArrayIndexVarICodeOp(int iFuncIndex, int iInstrIndex, int iArraySymbolIndex, int iOffsetSymbolIndex);
void AddFuncICodeOp(int iFuncIndex, int iInstrIndex, int iOpFuncIndex);
void AddRegICodeOp(int iFuncIndex, int iInstrIndex, int iRegCode);

Label DefineLabel();
void MarkLabel(int iFuncIndex, Label iTargetIndex);
void AddJumpTargetICodeOp(int iFuncIndex, int iInstrIndex, Label iTargetIndex);

#endif
