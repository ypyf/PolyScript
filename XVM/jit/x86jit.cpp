#include "x86jit.h"

// 对齐sz到align_sz
#define ALIGN_SIZE(sz, align_sz)				\
    (((sz) % (align_sz) != 0) ?			\
    ((align_sz) + (sz) - ((sz) % (align_sz))) : \
    (sz))

template <typename T>
T align_size(T sz, T align_sz)
{
    return (sz%align_sz != 0) ?	
           (align_sz+sz - sz%align_sz) : sz;
}

// 栈内临时变量
typedef uint32_t jit_tmp_t;

// 临时变量存储位置
enum jit_tmp_loc {
    JITLOC_UNALLOCATED,
    JITLOC_CONST,
    JITLOC_STACK,
    JITLOC_REG
};

// 临时变量信息
typedef struct jit_tmp_state {
    unsigned int	dirty  : 1;
    unsigned int	local  : 1;
    unsigned int	w64    : 1;
    unsigned int	pinned : 1;
    unsigned int	mem_allocated : 1;
    unsigned int	constant : 1;
    jit_tmp_loc	    loc;
    int		reg;
    int		mem_base_reg;
    int		mem_offset;
    int		id;

    int64_t	value;

    //struct jit_tmp_scan		    scan;
    //struct jit_tmp_out_scan		out_scan;
    //struct jit_tmp_call_info	call_info;
} *jit_tmp_state_t;



/************************
 * x86 指令操作码
 ************************/

#define OP_MOV_REG_RM		0x8B
#define OP_MOV_RM_REG		0x89
#define OP_MOV_RM_REG8		0x88
#define OP_MOV_REG_IMM32	0xB8
#define OP_MOV_RM_IMM32	    0xC7

#define OP_XOR_REG_RM		0x33
#define OP_XOR_RM_REG		0x31
#define OP_XOR_RM_IMM32	    0x81
#define OP_XOR_EAX_IMM8	    0x34
#define OP_XOR_EAX_IMM32	0x35
#define OP_XOR_RM_IMM8		0x80

#define OP_AND_REG_RM		0x23
#define OP_AND_RM_REG		0x21
#define OP_AND_RM_IMM32	    0x81
#define OP_AND_EAX_IMM8	    0x24
#define OP_AND_EAX_IMM32	0x25
#define OP_AND_RM_IMM8		0x80

#define OP_OR_REG_RM		0x0B
#define OP_OR_RM_REG		0x09
#define OP_OR_RM_IMM32		0x81
#define OP_OR_EAX_IMM8		0x0C
#define OP_OR_EAX_IMM32	    0x0D
#define OP_OR_RM_IMM8		0x80

// Add
#define OP_ADD_REG_RM       0x03
#define OP_ADD_RM_REG       0x01
#define OP_ADD_RM_IMM32     0x81
#define OP_ADD_EAX_IMM32    0x05
#define OP_ADD_RM_IMM8      0x83

#define OP_SUB_REG_RM		0x2B
#define OP_SUB_RM_REG		0x29
#define OP_SUB_RM_IMM32	    0x81
#define OP_SUB_EAX_IMM32	0x2D
#define OP_SUB_RM_IMM8		0x83

#define OP_ZEROF_PREFIX	    0x0F
#define OP_BSWAP_REG		0xC8
#define OP_BSF_REG			0xBC
#define OP_BSR_REG			0xBD

#define OP_CMP_REG_RM		0x3B
#define OP_CMP_RM_REG		0x39
#define OP_CMP_RM_IMM32	    0x81
#define OP_CMP_EAX_IMM32	0x3D

#define OP_MOVZX8		    0xB6
#define OP_MOVZX16		    0xB7

#define OP_MOVSX8		    0xBE
#define OP_MOVSX16		    0xBF
#define OP_MOVSX32		    0x63

#define OP_NOT_RM		    0xF7

#define OP_SHIFT_RM_IMM	    0xC1

#define OP_JMP			    0xE9
#define OP_JMP_REL8	        0xEB
#define OP_JMP_CC		    0x80
#define OP_JMP_CC_REL8	    0x70

// Call
#define OP_CALL_REL32       0xE8
#define OP_CALL_RM          0xFF

#define OP_INC			    0xFF
#define OP_DEC			    0xFF

#define OP_RET			    0xC3

#define OP_SET_CC		    0x90
#define OP_CMOV_CC		    0x40

#define OP_PUSH_REG         0x50
#define OP_PUSH_RM		    0xFF
#define OP_POP_REG		    0x58
#define OP_POP_RM		    0x8F
#define OP_ENTER		    0xC8
#define OP_LEAVE		    0xC9

#define OP_LZCNT		    0xBD

#define OP_XCHG_REG_RM		0x87
#define OP_XCHG_EAX_REG	    0x90

#define OP_SHIFT_RM_CL		0xD3
#define OP_TEST_RM_REG		0x85
#define OP_TEST_RM_IMM32	0xF7
#define OP_TEST_RM_IMM64	0xF7


/* ModR/M mod field values */
#define MODRM_MOD_DISP0		0x0     /* 00 没有位移 */
#define MODRM_MOD_DISP1B	0x1     /* 01 1字节位移 */
#define MODRM_MOD_DISP4B	0x2     /* 10 4字节位移 */
#define MODRM_MOD_RM_REG	0x3     /* 11 直接寻址 */

/* ModR/M reg field special values */
// 数值参看Intel手册中具体指令的 /digit
#define MODRM_REG_MOV_IMM	0x0     /* 000 000 */
#define MODRM_REG_AND_IMM	0x4     /* 000 100 */
#define MODRM_REG_CMP_IMM	0x7     /* 000 111 */
#define MODRM_REG_OR_IMM	0x1
#define MODRM_REG_XOR_IMM	0x6
#define MODRM_REG_SHL		0x4
#define MODRM_REG_SHR		0x5
#define MODRM_REG_ADD_IMM	0x0     /* /0 */
#define MODRM_REG_SUB_IMM	0x5
#define MODRM_REG_NOT		0x2
#define MODRM_REG_SETCC		0x0
#define MODRM_REG_INC		0x0
#define MODRM_REG_DEC		0x1
#define MODRM_REG_CALL_NEAR	0x2
#define MODRM_REG_PUSH		0x6
#define MODRM_REG_POP		0x0

// X86 register encoding values
enum x86_reg {
    REG_RAX = 0,
    REG_RCX,
    REG_RDX,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_CNT
};

// 可作为函数参数的寄存器
static const int fn_arg_regs[] = {
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9
};
#define FN_ARG_REG_CNT  (int)((sizeof(fn_arg_regs)/sizeof(int)))

// 函数原型
static int32_t calculate_disp32(uintptr_t tgt, uintptr_t reloc);
static int8_t calculate_disp8(uintptr_t tgt, uintptr_t reloc);
static int check_pc_rel32s(uint8_t *emit_ptr, uint64_t imm);
static void jit_emit(JitContext *ctx, uint8_t value);
static void jit_emit_i32(JitContext *ctx, int32_t value);
static void jit_emit_i64(JitContext *ctx, int64_t value);
static void jit_emit_rex(JitContext *ctx, int w64, int r_reg_ext, int x_sib_ext, int b_rm_ext);
static void jit_emit_opc1_reg_regrm(JitContext *ctx, int w64, int zerof_prefix, int force_rex, uint8_t opc, uint8_t reg, uint8_t rm_reg);
static void jit_emit_opc1_reg_regrm2(JitContext *ctx, int w64, int f3_prefix, int zerof_prefix, int force_rex, uint8_t opc, uint8_t reg, uint8_t rm_reg);
static void jit_emit_add_reg_imm32(JitContext *ctx, int w64, uint8_t rm_reg, int32_t imm);

/*
    计算函数相对地址
    tgt 表示要计算的函数地址
    reloc 表示计算后的相对地址将要发送到的位置
    返回值是 rel32
*/
static int32_t calculate_disp32(uintptr_t tgt, uintptr_t reloc)
{
    uintptr_t diff;
    int32_t disp;

    if (tgt > reloc) 
    {
        // 由于是计算的是相对于下一条指令的位移量，所以还要减去地址本身所占的4个字节
        diff = tgt - reloc - 4; 
        disp = (int32_t)diff;
    } 
    else 
    {
        diff = reloc - tgt + 4;
        disp = -(int32_t)diff;
    }

    //assert (diff <= INT32_MAX);

    return disp;
}

static int8_t calculate_disp8(uintptr_t tgt, uintptr_t reloc)
{
    uintptr_t diff;
    int8_t disp;

    if ((uintptr_t)tgt > (uintptr_t)reloc) {
        diff = (uintptr_t)tgt - (uintptr_t)reloc - 1;
        disp = (int8_t)diff;
    } else {
        diff = (uintptr_t)reloc - (uintptr_t)tgt + 1;
        disp = -(int8_t)diff;
    }

    //assert (diff <= INT8_MAX);

    return disp;
}

// 检查函数位移量是否在32位有符号数的范围内
static int check_pc_rel32s(uint8_t *emit_ptr, uint64_t imm)
{
    uint64_t approx_pc;
    uint64_t diff;
    uint32_t diff_lim;

    //assert (ctx->codebuf != NULL);

    // 当前代码地址
    approx_pc = (uint64_t)emit_ptr;

    // 计算立即数与当前代码地址的距离（位移量）
    if (imm > approx_pc)
        diff = imm - approx_pc;
    else
        diff = approx_pc - imm;

    /* 4K safety buffer */
    // 安全起见，假设实际发射指令距离可能更大?
    diff += 4096;

    // 检查位移量是否超过了一个有符号整数
    diff_lim = (uint32_t)diff;
    diff_lim &= 0x7FFFFFFF;
    return ((uint64_t)diff_lim == diff);
}

// 函数定义
static void jit_emit(JitContext *ctx, uint8_t value)
{
    ctx->emit_ptr[0] = value;
    ctx->emit_ptr++;
    ctx->code_size++;
}

static void jit_emit_i32(JitContext *ctx, int32_t value)
{
    ((int32_t *)ctx->emit_ptr)[0] = value;
    ctx->emit_ptr += sizeof(int32_t);
    ctx->code_size += sizeof(int32_t);
}

static void jit_emit_i64(JitContext *ctx, int64_t value)
{
    ((int64_t *)ctx->emit_ptr)[0] = value;
    ctx->emit_ptr += sizeof(int64_t);
    ctx->code_size += sizeof(int64_t);
}

// 发射 REX 操作数前缀
static void jit_emit_rex(JitContext *ctx, int w64, int r_reg_ext, int x_sib_ext, int b_rm_ext)
{
    uint8_t rex;

    rex = (0x4 << 4) |  // 高4位是固定模式 0100
        ((w64       ? 1 : 0) << 3) |    // 64位操作数前缀
        ((r_reg_ext ? 1 : 0) << 2) |    // 指示这四个寄存器: SPL,BPL,SIL,DIL
        ((x_sib_ext ? 1 : 0) << 1) |    // SIB相关
        ((b_rm_ext  ? 1 : 0) << 0);     // SIB相关

    jit_emit(ctx, rex);
}


static void jit_emit_opc1_reg_rm2(JitContext *ctx, int w64, int f3_prefix, int zerof_prefix, int force_rex, uint8_t opc, uint8_t reg, uint8_t rm)
{
    // ModR/M
    uint8_t modrm;
    // 提取reg的第四位
    // 如果reg_ext为1，说明是x64中的R8~R15这几个寄存器
    int reg_ext = reg & 0x8;
    // 提取rm_reg的第四位
    int rm_ext = rm & 0x8;

    // reg和rm_reg字段各占3位
    reg &= 0x7;
    rm &= 0x7;

    // 发射REX前缀
    if (reg_ext || rm_ext || w64 || force_rex)
        jit_emit_rex(ctx, w64, reg_ext, 0, rm_ext);

    // 寄存器直接寻址
    modrm = (MODRM_MOD_RM_REG << 6) | (reg << 3) | (rm);

    if (f3_prefix)
        jit_emit(ctx, 0xF3);

    if (zerof_prefix)
        jit_emit(ctx, OP_ZEROF_PREFIX);

    jit_emit(ctx, opc);
    jit_emit(ctx, modrm);
}

// 发射指令(I型， 寄存器直接寻址)
static void jit_emit_opc1_reg_rm(JitContext *ctx, int w64, int zerof_prefix, int force_rex, uint8_t opc, uint8_t reg, uint8_t rm)
{
    // 0x0F是个啥？
    jit_emit_opc1_reg_rm2(ctx, w64, 0, zerof_prefix, force_rex, opc, reg, rm);
}

// add reg, imm8
// add reg, imm32
static void jit_emit_add_reg_imm32(JitContext *ctx, int w64, uint8_t rm_reg, int32_t imm)
{
    int8_t imm8 = (int8_t)imm;
    int use_imm8 = ((int32_t)imm8 == imm);

    if (rm_reg == REG_RAX) {
        jit_emit(ctx, OP_ADD_EAX_IMM32);
    } 
    else {
        jit_emit_opc1_reg_rm(ctx, w64, 0, 0,
            use_imm8 ? OP_ADD_RM_IMM8 : OP_ADD_RM_IMM32,
            MODRM_REG_ADD_IMM, rm_reg);
    }

    use_imm8 ? jit_emit(ctx, imm8) : jit_emit_i32(ctx, imm);
}

/******************************************************************************************
*
*  jit_emit_call() - 发射函数调用指令
*
*  params[0]: function pointer
*  params[1]: output temp
*  params[2..cnt-1]: input temps
*/
void jit_x86_emit_call(JitContext *ctx, int cnt, uint64_t *params)
{    
    // 参数的数量
    int in_temps = (cnt - 2);   
    // 寄存器溢出所需堆栈位移量
    int32_t stack_disp = (in_temps > FN_ARG_REG_CNT) ?
        (in_temps - FN_ARG_REG_CNT)*sizeof(uint64_t) : 0;
    // 函数地址
    uintptr_t fn_ptr = (uintptr_t)params[0];
    // 函数地址位移量（相对地址rel32）
    int32_t disp32;
    // ModR/M
    //uint8_t modrm;

    // 栈空间对齐到32位
    stack_disp = ALIGN_SIZE(stack_disp, 32);

    // 如果函数位移量在32位有符号数范围内
    if (check_pc_rel32s(ctx->emit_ptr, fn_ptr)) {
        // 使用RIP相对地址调用
        jit_emit(ctx, OP_CALL_REL32);
        // 计算相对位移
        disp32 = calculate_disp32(fn_ptr, (uintptr_t)ctx->emit_ptr);
        jit_emit_i32(ctx, disp32);
    }
    // 否则通过R/M进行间接调用
    //else {
    //    /* Move into a scratch register first */
    //    if (check_unsigned32(fn_ptr))
    //        jit_emit_mov_reg_imm32(ctx->codebuf, REG_RAX, fn_ptr);
    //    else
    //        jit_emit_mov_reg_imm64(ctx->codebuf, REG_RAX, fn_ptr);

    //    jit_emit_opc1_reg_regrm(ctx->codebuf, 0, 0, 0, OP_CALL_RM,
    //        MODRM_REG_CALL_NEAR, REG_RAX);
    //}

    /* 恢复堆栈平衡 */
    if (stack_disp > 0)
        jit_emit_add_reg_imm32(ctx, TRUE, REG_RSP, stack_disp);
}


/******************************************************************************************
*
*  jit_x86_emit_fn_prologue() - 发射函数头部
*
*  cnt: the number of params
*  params: input temps
*/
//void jit_x86_emit_fn_prologue(JitContext *ctx, int cnt, uint64_t *params)
//{
//    jit_tmp_t tmp;
//    jit_tmp_state_t ts;
//    int i;
//    int mem_offset = 0x10;
//
//    // 依次处理每一个参数
//    for (i = 0; i < cnt; i++) {
//        tmp = (jit_tmp_t)params[i];
//        ts = GET_TMP_STATE(ctx, tmp);
//
//        if (i < FN_ARG_REG_CNT) {
//            ts->loc = JITLOC_REG;
//            ts->mem_allocated = 0;
//            ts->reg = fn_arg_regs[i];
//            ts->dirty = 1;
//            jit_regset_set(ctx->regs_used, ts->reg);
//            ctx->reg_to_tmp[ts->reg] = tmp;
//        }
//        else {
//            ts->loc = JITLOC_STACK;
//            ts->mem_allocated = 1;
//            ts->mem_base_reg = jit_tgt_stack_base_reg;
//            ts->mem_offset = mem_offset;
//
//            mem_offset += sizeof(uint64_t);
//        }
//    }
//
//#ifdef JIT_FPO
//#else
//    ctx->spill_stack_offset = -((int)sizeof(uint64_t)*(CALLEE_SAVED_REG_CNT+1));
//#endif
//
//    ctx->emit_ptr = ctx->func_ptr = ctx->func_ptr + MAX_PROLOGUE_SZ;
//}


