#ifndef __BYTECODE_H__
#define __BYTECODE_H__

// ----Instruction Opcodes -----------------------------------------------------------

enum Opcodes {
	INSTR_NOP = 0,
	INSTR_BREAK,
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
	INSTR_JMP,
	INSTR_JE,
	INSTR_JNE,
	INSTR_JG,
	INSTR_JL,
	INSTR_JGE,
	INSTR_JLE,
	INSTR_BRTRUE,
	INSTR_BRFALSE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_DUP,
	INSTR_REMOVE,
	INSTR_CALL,
	INSTR_RET,
	INSTR_PAUSE,
	INSTR_ICONST0,			// push 0
	INSTR_ICONST1,			// push 1
	INSTR_FCONST_0,			// push 0.f
	INSTR_FCONST_1,			// push 1.f
	INSTR_TRAP,
	INSTR_SQRT,
	INSTR_NEW,
	INSTR_THISCALL,
	INSTR_HALT,
};

#endif /* __BYTECODE_H__ */
