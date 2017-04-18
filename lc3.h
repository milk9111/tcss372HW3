/*
	Author: Connor Lundberg, James Roberts
	Date: 4/14/2017
*/

#define FETCH 0
#define DECODE 1
#define EVAL_ADDR 2
#define FETCH_OP 3
#define EXECUTE 4
#define STORE 5

#define ADD 1
#define ADI 1
#define AND 5
#define ANI 5
#define NOT 9
#define TRAP 15
#define LD 2
#define ST 3
#define JMP 12
#define BR 0

#define NO_OF_REGISTERS 8

#define OPCODE_FIELD 0xF000				// 1111 0000 0000 0000 - gets first three bits
#define OPCODE_FIELD_SHIFT 12
#define RD_FIELD 0x0E00					// 0000 1110 0000 0000 - gets Rd field bits
#define RD_FIELD_SHIFT 9
#define RS1_FIELD 0x01C0				// 0000 0001 1100 0000 - gets Rs1 field bits
#define RS1_FIELD_SHIFT 6
#define RS2_FIELD 0x0007				// 0000 0000 0000 0111 - gets Rs2 field bits
#define RS2_FIELD_SHIFT 0
#define IMMED5_FIELD 0x001F				// 0000 0000 0011 1111 - gets immed5 field bits
#define IMMED5_FIELD_SHIFT 0
#define BR_FIELD 0x0E00					// 0000 1110 0000 0000 - gets branch field bits
#define BR_FIELD_SHIFT 9
#define OFFSET9_FIELD 0x01FF			// 0000 0001 1111 1111 - gets offset9 field bits
#define OFFSET9_FIELD_SHIFT 0
#define TRAP_VECTOR8_FIELD 0x00FF		// 0000 0000 1111 1111 - gets trap vector 8 bits
#define TRAP_VECTOR8_FIELD_SHIFT 0
#define HIGH_ORDER_BIT_VALUE 0x0020		// 0000 0000 0010 0000
#define HIGH_ORDER_BIT_VALUE9 0x0100	// 0000 0001 0000 0000
#define END 0x0E14


typedef unsigned short Register;

typedef struct alu_s {
	Register a;
	Register b;
	Register r;
} ALU_s;

typedef ALU_s *ALU_p;

typedef struct cpu_s {
	Register ir;
	Register pc;
	Register mar;
	Register mdr;
	Register reg_file[NO_OF_REGISTERS];
	int n, z, p;
} CPU_s;

typedef CPU_s *CPU_p;

