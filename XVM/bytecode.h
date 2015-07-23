#ifndef BYTE_CODE_H_
#define BYTE_CODE_H_

// ----Instruction Opcodes -----------------------------------------------------------

enum InstrSet {
	INSTR_NOP = 0,
	INSTR_MOV,
	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_DIV,
	INSTR_MOD,
	INSTR_EXP,
	INSTR_NEG,
	INSTR_INC,
	INSTR_DEC,
	INSTR_AND,
	INSTR_OR,
	INSTR_XOR,
	INSTR_NOT,
	INSTR_SHL,
	INSTR_SHR,
	INSTR_CONCAT,
	INSTR_GETCHAR,
	INSTR_SETCHAR,
	INSTR_JMP,
	INSTR_JE,
	INSTR_JNE,
	INSTR_JG,
	INSTR_JL,
	INSTR_JGE,
	INSTR_JLE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_CALL,
	INSTR_RET,
	INSTR_PAUSE,
	INSTR_ICONST_0,			// push 0
	INSTR_ICONST_1,			// push 1
	INSTR_FCONST_0,			// push 0.f
	INSTR_FCONST_1,			// push 1.f
	INSTR_PRINT,
	INSTR_BREAK,
	INSTR_SQRT,
	INSTR_HALT = 512,
	INSTR_NEW = 128,
	INSTR_THISCALL = 129,
};

//#define INSTR_DEL             0x52
//#define INSTR_GET_PROP        0x53
//#define INSTR_SET_PROP        0x54


#endif /* BYTE_CODE_H_ */
