#ifdef __ARM_EABI__

#include "ibm.h"
#include "codegen.h"
#include "codegen_backend.h"
#include "codegen_backend_arm_defs.h"
#include "codegen_backend_arm_ops.h"

static inline void codegen_addlong(codeblock_t *block, uint32_t val)
{
        *(uint32_t *)&block->data[block_pos] = val;
        block_pos += 4;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addlong over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}

#define Rm(x) (x)
#define Rs(x) ((x) << 8)
#define Rd(x) ((x) << 12)
#define Rn(x) ((x) << 16)

#define DATA_OFFSET_UP   (1 << 23)
#define DATA_OFFSET_DOWN (0 << 23)

#define COND_SHIFT 28
#define COND_EQ (0x0 << COND_SHIFT)
#define COND_NE (0x1 << COND_SHIFT)
#define COND_AL (0xe << COND_SHIFT)

#define OPCODE_SHIFT 20
#define OPCODE_ADD_IMM  (0x28 << OPCODE_SHIFT)
#define OPCODE_ADD_REG  (0x08 << OPCODE_SHIFT)
#define OPCODE_AND_IMM  (0x20 << OPCODE_SHIFT)
#define OPCODE_AND_REG  (0x00 << OPCODE_SHIFT)
#define OPCODE_B        (0xa0 << OPCODE_SHIFT)
#define OPCODE_BIC_IMM  (0x3c << OPCODE_SHIFT)
#define OPCODE_BIC_REG  (0x1c << OPCODE_SHIFT)
#define OPCODE_BL       (0xb0 << OPCODE_SHIFT)
#define OPCODE_CMN_IMM  (0x37 << OPCODE_SHIFT)
#define OPCODE_CMN_REG  (0x17 << OPCODE_SHIFT)
#define OPCODE_CMP_IMM  (0x35 << OPCODE_SHIFT)
#define OPCODE_CMP_REG  (0x15 << OPCODE_SHIFT)
#define OPCODE_EOR_IMM  (0x22 << OPCODE_SHIFT)
#define OPCODE_EOR_REG  (0x02 << OPCODE_SHIFT)
#define OPCODE_LDMIA_WB (0x8b << OPCODE_SHIFT)
#define OPCODE_LDR_IMM  (0x51 << OPCODE_SHIFT)
#define OPCODE_LDR_IMM_POST  (0x41 << OPCODE_SHIFT)
#define OPCODE_LDR_REG  (0x79 << OPCODE_SHIFT)
#define OPCODE_LDRB_IMM (0x55 << OPCODE_SHIFT)
#define OPCODE_LDRB_REG (0x7d << OPCODE_SHIFT)
#define OPCODE_MOV_IMM  (0x3a << OPCODE_SHIFT)
#define OPCODE_MOVT_IMM (0x34 << OPCODE_SHIFT)
#define OPCODE_MOVW_IMM (0x30 << OPCODE_SHIFT)
#define OPCODE_MOV_REG  (0x1a << OPCODE_SHIFT)
#define OPCODE_MVN_REG  (0x1e << OPCODE_SHIFT)
#define OPCODE_ORR_IMM  (0x38 << OPCODE_SHIFT)
#define OPCODE_ORR_REG  (0x18 << OPCODE_SHIFT)
#define OPCODE_RSB_REG  (0x06 << OPCODE_SHIFT)
#define OPCODE_STMDB_WB (0x92 << OPCODE_SHIFT)
#define OPCODE_STR_IMM  (0x50 << OPCODE_SHIFT)
#define OPCODE_STR_IMM_WB  (0x52 << OPCODE_SHIFT)
#define OPCODE_STR_REG  (0x78 << OPCODE_SHIFT)
#define OPCODE_STRB_IMM (0x54 << OPCODE_SHIFT)
#define OPCODE_STRB_REG (0x7c << OPCODE_SHIFT)
#define OPCODE_SUB_IMM  (0x24 << OPCODE_SHIFT)
#define OPCODE_SUB_REG  (0x04 << OPCODE_SHIFT)
#define OPCODE_TST_IMM  (0x31 << OPCODE_SHIFT)
#define OPCODE_TST_REG  (0x11 << OPCODE_SHIFT)

#define OPCODE_BFI    0xe7c00010
#define OPCODE_BLX    0xe12fff30
#define OPCODE_LDRH_IMM 0xe1d000b0
#define OPCODE_LDRH_REG 0xe19000b0
#define OPCODE_STRH_IMM 0xe1c000b0
#define OPCODE_STRH_REG 0xe18000b0
#define OPCODE_UADD8  0xe6500f90
#define OPCODE_UADD16 0xe6500f10
#define OPCODE_USUB8  0xe6500ff0
#define OPCODE_USUB16 0xe6500f70
#define OPCODE_UXTB   0xe6ef0070
#define OPCODE_UXTH   0xe6ff0070

#define B_OFFSET(x) (((x) >> 2) & 0xffffff)

#define SHIFT_TYPE_SHIFT 5
#define SHIFT_TYPE_LSL (0 << SHIFT_TYPE_SHIFT)
#define SHIFT_TYPE_LSR (1 << SHIFT_TYPE_SHIFT)
#define SHIFT_TYPE_ASR (2 << SHIFT_TYPE_SHIFT)
#define SHIFT_TYPE_ROR (3 << SHIFT_TYPE_SHIFT)

#define SHIFT_TYPE_IMM (0 << 4)
#define SHIFT_TYPE_REG (1 << 4)

#define SHIFT_IMM_SHIFT 7
#define SHIFT_ASR_IMM(x) (SHIFT_TYPE_ASR | SHIFT_TYPE_IMM | ((x) << SHIFT_IMM_SHIFT))
#define SHIFT_LSL_IMM(x) (SHIFT_TYPE_LSL | SHIFT_TYPE_IMM | ((x) << SHIFT_IMM_SHIFT))
#define SHIFT_LSR_IMM(x) (SHIFT_TYPE_LSR | SHIFT_TYPE_IMM | ((x) << SHIFT_IMM_SHIFT))

#define SHIFT_ASR_REG(x) (SHIFT_TYPE_ASR | SHIFT_TYPE_REG | Rs(x))
#define SHIFT_LSL_REG(x) (SHIFT_TYPE_LSL | SHIFT_TYPE_REG | Rs(x))
#define SHIFT_LSR_REG(x) (SHIFT_TYPE_LSR | SHIFT_TYPE_REG | Rs(x))

#define BFI_lsb(lsb) ((lsb) << 7)
#define BFI_msb(msb) ((msb) << 16)

#define UXTB_ROTATE(rotate) (((rotate) >> 3) << 10)

#define MOVT_IMM(imm) (((imm) & 0xfff) | (((imm) & 0xf000) << 4))
#define MOVW_IMM(imm) (((imm) & 0xfff) | (((imm) & 0xf000) << 4))

#define LDRH_IMM(imm) (((imm) & 0xf) | (((imm) & 0xf0) << 4))
#define STRH_IMM(imm) LDRH_IMM(imm)


static inline uint32_t arm_data_offset(int offset)
{
	if (offset < -0xffc || offset > 0xffc)
		fatal("arm_data_offset out of range - %i\n", offset);

	if (offset >= 0)
		return offset | DATA_OFFSET_UP;
	return (-offset) | DATA_OFFSET_DOWN;
}

static inline int get_arm_imm(uint32_t imm_data, uint32_t *arm_imm)
{
	int shift = 0;
//pclog("get_arm_imm - imm_data=%08x\n", imm_data);
	if (!(imm_data & 0xffff))
	{
		shift += 16;
		imm_data >>= 16;
	}
	if (!(imm_data & 0xff))
	{
		shift += 8;
		imm_data >>= 8;
	}
	if (!(imm_data & 0xf))
	{
		shift += 4;
		imm_data >>= 4;
	}
	if (!(imm_data & 0x3))
	{
		shift += 2;
		imm_data >>= 2;
	}
	if (imm_data > 0xff) /*Note - should handle rotation round the word*/
		return 0;
//pclog("   imm_data=%02x shift=%i\n", imm_data, shift);
	*arm_imm = imm_data | ((((32 - shift) >> 1) & 15) << 8);
	return 1;
}

static inline int in_range(void *addr, void *base)
{
	int diff = (uintptr_t)addr - (uintptr_t)base;

	if (diff < -4095 || diff > 4095)
		return 0;
	return 1;
}

void host_arm_ADD_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift);
void host_arm_AND_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift);
void host_arm_EOR_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift);
void host_arm_ORR_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift);
void host_arm_SUB_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift);

void host_arm_ADD_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if ((int32_t)imm < 0 && imm != 0x80000000)
	{
		host_arm_SUB_IMM(block, dst_reg, src_reg, -(int32_t)imm);
	}
	else if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_ADD_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_ADD_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}

void host_arm_ADD_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_ADD_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}
void host_arm_ADD_REG_LSR(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_ADD_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSR_IMM(shift));
}

void host_arm_AND_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_AND_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else if (get_arm_imm(~imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_BIC_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_AND_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}

void host_arm_AND_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_AND_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}
void host_arm_AND_REG_LSR(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_AND_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSR_IMM(shift));
}

void host_arm_BFI(codeblock_t *block, int dst_reg, int src_reg, int lsb, int width)
{
	codegen_addlong(block, OPCODE_BFI | Rd(dst_reg) | Rm(src_reg) | BFI_lsb(lsb) | BFI_msb((lsb + width) - 1));
}

void host_arm_BIC_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_BIC_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else if (get_arm_imm(~imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_AND_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_BIC_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}
void host_arm_BIC_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_BIC_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}
void host_arm_BIC_REG_LSR(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_BIC_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSR_IMM(shift));
}

void host_arm_BL(codeblock_t *block, uintptr_t dest_addr)
{
	uint32_t offset = (dest_addr - (uintptr_t)&block->data[block_pos]) - 8;

	if ((offset & 0xfe000000) && (offset & 0xfe000000) != 0xfe000000)
	{
		host_arm_MOV_IMM(block, REG_R3, dest_addr);
		host_arm_BLX(block, REG_R3);
	}
	else
		codegen_addlong(block, COND_AL | OPCODE_BL | B_OFFSET(offset));
}
void host_arm_BLX(codeblock_t *block, int addr_reg)
{
	codegen_addlong(block, OPCODE_BLX | Rm(addr_reg));
}

uint32_t *host_arm_BEQ_(codeblock_t *block)
{
	codegen_addlong(block, COND_EQ | OPCODE_B);

	return (uint32_t *)&block->data[block_pos - 4];
}
uint32_t *host_arm_BNE_(codeblock_t *block)
{
	codegen_addlong(block, COND_NE | OPCODE_B);

	return (uint32_t *)&block->data[block_pos - 4];
}

void host_arm_BEQ(codeblock_t *block, uintptr_t dest_addr)
{
	uint32_t offset = (dest_addr - (uintptr_t)&block->data[block_pos]) - 8;

	if ((offset & 0xfe000000) && (offset & 0xfe000000) != 0xfe000000)
		fatal("host_arm_BEQ - out of range %08x %i\n", offset, offset);

	codegen_addlong(block, COND_EQ | OPCODE_B | B_OFFSET(offset));
}
void host_arm_BNE(codeblock_t *block, uintptr_t dest_addr)
{
	uint32_t offset = (dest_addr - (uintptr_t)&block->data[block_pos]) - 8;

	if ((offset & 0xfe000000) && (offset & 0xfe000000) != 0xfe000000)
		fatal("host_arm_BNE - out of range %08x %i\n", offset, offset);

	codegen_addlong(block, COND_NE | OPCODE_B | B_OFFSET(offset));
}

void host_arm_CMN_IMM(codeblock_t *block, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if ((int32_t)imm < 0 && imm != 0x80000000)
	{
		host_arm_CMP_IMM(block, src_reg, -(int32_t)imm);
	}
	else if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_CMN_IMM | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_CMN_REG_LSL(block, src_reg, REG_TEMP, 0);
	}
}
void host_arm_CMN_REG_LSL(codeblock_t *block, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_CMN_REG | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}

void host_arm_CMP_IMM(codeblock_t *block, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if ((int32_t)imm < 0 && imm != 0x80000000)
	{
		host_arm_CMN_IMM(block, src_reg, -(int32_t)imm);
	}
	else if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_CMP_IMM | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_CMP_REG_LSL(block, src_reg, REG_TEMP, 0);
	}
}
void host_arm_CMP_REG_LSL(codeblock_t *block, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_CMP_REG | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}

void host_arm_EOR_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_EOR_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_EOR_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}

void host_arm_EOR_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_EOR_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}

void host_arm_LDMIA_WB(codeblock_t *block, int addr_reg, uint32_t reg_mask)
{
	codegen_addlong(block, COND_AL | OPCODE_LDMIA_WB | Rn(addr_reg) | reg_mask);
}

void host_arm_LDR_IMM(codeblock_t *block, int dst_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_LDR_IMM | Rn(addr_reg) | Rd(dst_reg) | arm_data_offset(offset));
}
void host_arm_LDR_IMM_POST(codeblock_t *block, int dst_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_LDR_IMM_POST | Rn(addr_reg) | Rd(dst_reg) | arm_data_offset(offset));
}
void host_arm_LDR_REG_LSL(codeblock_t *block, int dst_reg, int addr_reg, int offset_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_LDR_REG | Rn(addr_reg) | Rd(dst_reg) | Rm(offset_reg) | SHIFT_LSL_IMM(shift));
}

void host_arm_LDRB_ABS(codeblock_t *block, int dst_reg, void *p)
{
	if (in_range(p, &cpu_state))
		host_arm_LDRB_IMM(block, dst_reg, REG_CPUSTATE, (uintptr_t)p - (uintptr_t)&cpu_state);
	else
		fatal("LDRB_ABS - not in range\n");
}
void host_arm_LDRB_IMM(codeblock_t *block, int dst_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_LDRB_IMM | Rn(addr_reg) | Rd(dst_reg) | arm_data_offset(offset));
}
void host_arm_LDRB_REG_LSL(codeblock_t *block, int dst_reg, int addr_reg, int offset_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_LDRB_REG | Rn(addr_reg) | Rd(dst_reg) | Rm(offset_reg) | SHIFT_LSL_IMM(shift));
}

void host_arm_LDRH_IMM(codeblock_t *block, int dst_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_LDRH_IMM | Rn(addr_reg) | Rd(dst_reg) | LDRH_IMM(offset));
}
void host_arm_LDRH_REG(codeblock_t *block, int dst_reg, int addr_reg, int offset_reg)
{
	codegen_addlong(block, COND_AL | OPCODE_LDRH_REG | Rn(addr_reg) | Rd(dst_reg) | Rm(offset_reg));
}

void host_arm_MOV_IMM(codeblock_t *block, int dst_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_MOV_IMM | Rd(dst_reg) | arm_imm);
	}
	else
	{
		host_arm_MOVW_IMM(block, dst_reg, imm & 0xffff);
		if (imm >> 16)
			host_arm_MOVT_IMM(block, dst_reg, imm >> 16);
	}
}

void host_arm_MOV_REG_ASR(codeblock_t *block, int dst_reg, int src_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_ASR_IMM(shift));
}
void host_arm_MOV_REG_ASR_REG(codeblock_t *block, int dst_reg, int src_reg, int shift_reg)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_ASR_REG(shift_reg));
}
void host_arm_MOV_REG_LSL(codeblock_t *block, int dst_reg, int src_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_LSL_IMM(shift));
}
void host_arm_MOV_REG_LSL_REG(codeblock_t *block, int dst_reg, int src_reg, int shift_reg)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_LSL_REG(shift_reg));
}
void host_arm_MOV_REG_LSR(codeblock_t *block, int dst_reg, int src_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_LSR_IMM(shift));
}
void host_arm_MOV_REG_LSR_REG(codeblock_t *block, int dst_reg, int src_reg, int shift_reg)
{
	codegen_addlong(block, COND_AL | OPCODE_MOV_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_LSR_REG(shift_reg));
}

void host_arm_MOVT_IMM(codeblock_t *block, int dst_reg, uint16_t imm)
{
	codegen_addlong(block, COND_AL | OPCODE_MOVT_IMM | Rd(dst_reg) | MOVT_IMM(imm));
}
void host_arm_MOVW_IMM(codeblock_t *block, int dst_reg, uint16_t imm)
{
	codegen_addlong(block, COND_AL | OPCODE_MOVW_IMM | Rd(dst_reg) | MOVW_IMM(imm));
}

void host_arm_MVN_REG_LSL(codeblock_t *block, int dst_reg, int src_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_MVN_REG | Rd(dst_reg) | Rm(src_reg) | SHIFT_LSL_IMM(shift));
}

void host_arm_ORR_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_ORR_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_ORR_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}

void host_arm_ORR_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_ORR_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}

void host_arm_RSB_REG_LSR(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_RSB_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSR_IMM(shift));
}

void host_arm_STMDB_WB(codeblock_t *block, int addr_reg, uint32_t reg_mask)
{
	codegen_addlong(block, COND_AL | OPCODE_STMDB_WB | Rn(addr_reg) | reg_mask);
}

void host_arm_STR_IMM(codeblock_t *block, int src_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_STR_IMM | Rn(addr_reg) | Rd(src_reg) | arm_data_offset(offset));
}
void host_arm_STR_IMM_WB(codeblock_t *block, int src_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_STR_IMM_WB | Rn(addr_reg) | Rd(src_reg) | arm_data_offset(offset));
}
void host_arm_STR_REG_LSL(codeblock_t *block, int src_reg, int addr_reg, int offset_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_STR_REG | Rn(addr_reg) | Rd(src_reg) | Rm(offset_reg) | SHIFT_LSL_IMM(shift));
}

void host_arm_STRB_IMM(codeblock_t *block, int src_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_STRB_IMM | Rn(addr_reg) | Rd(src_reg) | arm_data_offset(offset));
}
void host_arm_STRB_REG_LSL(codeblock_t *block, int src_reg, int addr_reg, int offset_reg, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_STRB_REG | Rn(addr_reg) | Rd(src_reg) | Rm(offset_reg) | SHIFT_LSL_IMM(shift));
}

void host_arm_STRH_IMM(codeblock_t *block, int dst_reg, int addr_reg, int offset)
{
	codegen_addlong(block, COND_AL | OPCODE_STRH_IMM | Rn(addr_reg) | Rd(dst_reg) | STRH_IMM(offset));
}
void host_arm_STRH_REG(codeblock_t *block, int src_reg, int addr_reg, int offset_reg)
{
	codegen_addlong(block, COND_AL | OPCODE_STRH_REG | Rn(addr_reg) | Rd(src_reg) | Rm(offset_reg));
}

void host_arm_SUB_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if ((int32_t)imm < 0 && imm != 0x80000000)
	{
		host_arm_ADD_IMM(block, dst_reg, src_reg, -(int32_t)imm);
	}
	else if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_SUB_IMM | Rd(dst_reg) | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_SUB_REG_LSL(block, dst_reg, src_reg, REG_TEMP, 0);
	}
}

void host_arm_SUB_REG_LSL(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_SUB_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSL_IMM(shift));
}
void host_arm_SUB_REG_LSR(codeblock_t *block, int dst_reg, int src_reg_n, int src_reg_m, int shift)
{
	codegen_addlong(block, COND_AL | OPCODE_SUB_REG | Rd(dst_reg) | Rn(src_reg_n) | Rm(src_reg_m) | SHIFT_LSR_IMM(shift));
}

void host_arm_TST_IMM(codeblock_t *block, int src_reg, uint32_t imm)
{
	uint32_t arm_imm;

	if (get_arm_imm(imm, &arm_imm))
	{
		codegen_addlong(block, COND_AL | OPCODE_TST_IMM | Rn(src_reg) | arm_imm);
	}
	else
	{
		host_arm_MOV_IMM(block, REG_TEMP, imm);
		host_arm_TST_REG(block, src_reg, REG_TEMP);
	}
}
void host_arm_TST_REG(codeblock_t *block, int src_reg1, int src_reg2)
{
	codegen_addlong(block, COND_AL | OPCODE_TST_REG | Rn(src_reg1) | Rm(src_reg2));
}

void host_arm_UADD8(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
	codegen_addlong(block, COND_AL | OPCODE_UADD8 | Rd(dst_reg) | Rn(src_reg_a) | Rm(src_reg_b));
}

void host_arm_UADD16(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
	codegen_addlong(block, COND_AL | OPCODE_UADD16 | Rd(dst_reg) | Rn(src_reg_a) | Rm(src_reg_b));
}

void host_arm_USUB8(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
	codegen_addlong(block, COND_AL | OPCODE_USUB8 | Rd(dst_reg) | Rn(src_reg_a) | Rm(src_reg_b));
}

void host_arm_USUB16(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
	codegen_addlong(block, COND_AL | OPCODE_USUB16 | Rd(dst_reg) | Rn(src_reg_a) | Rm(src_reg_b));
}

void host_arm_UXTB(codeblock_t *block, int dst_reg, int src_reg, int rotate)
{
	codegen_addlong(block, OPCODE_UXTB | Rd(dst_reg) | Rm(src_reg) | UXTB_ROTATE(rotate));
}

void host_arm_UXTH(codeblock_t *block, int dst_reg, int src_reg, int rotate)
{
	codegen_addlong(block, OPCODE_UXTH | Rd(dst_reg) | Rm(src_reg) | UXTB_ROTATE(rotate));
}

#endif