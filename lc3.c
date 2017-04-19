/*
	Author: Connor Lundberg, Vincent Povio
	Date: 4/14/2017
	
	The first version of our LC3 personal simulation. It takes a single command
	and runs it through the controller, terminating with a HALT trap. We do this
	to end the loop and also to cut out the HALT trap test.
*/
#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

//current version


// you can define a simple memory module here for this program
unsigned int memory[32];   // 32 words of memory enough to store simple program
unsigned short memory_start = 0x3000;

void initializeCPU(CPU_p *, ALU_p *);
void display(CPU_p *, ALU_p *);
void setFlags(CPU_p *, ALU_p *, Register, Register);

// This is the trap function that handles trap vectors. Acts as 
// the trap vector table for now. Currently exits the HALT trap command.
void trap(int trap_vector) {
	//if (trap_vector == 0x0020) { //GETC
	//} else if (trap_vector == 0x0021) { //OUT
	//} else if (trap_vector == 0x0022) { //PUTS
	//} else if (trap_vector == 0x0023) { //IN
	//} else if (trap_vector == 0x0024) { //PUTSP
	if (trap_vector == 25) { //HALT
		exit(0);
	}
}


// This is the sext for the immed5. 
int sext5(int immed5) {
	if (HIGH_ORDER_BIT_VALUE & immed5) return (immed5 | 0xFFF0);
	else return immed5;
}


// This is the sext for the PCOffset9.
int sext9(int offset9) {
	if (HIGH_ORDER_BIT_VALUE9 & offset9) return (offset9 | 0xFD00);
	else return offset9;
}


// This is the main controller for our CPU. It is complete with all microstates
// defined for this project.
int controller (CPU_p cpu, ALU_p alu) {
    // check to make sure both pointers are not NULL
    // do any initializations here
	Register opcode, Rd, Rs1, Rs2, immed5, offset9;	// fields for the IR
	Register effective_addr, trapVector8, BaseR;
  initializeCPU(&cpu, &alu);
    int state = FETCH, BEN;
    for (;;) {
        switch (state) {
            case FETCH:
              cpu->mar = cpu->pc;
              cpu->pc++;
              cpu->mdr = memory[cpu->mar];
              cpu->ir = cpu->mdr;
              state = DECODE;
              break;
            case DECODE:
              opcode = (cpu->ir & OPCODE_FIELD) >> OPCODE_FIELD_SHIFT;
              switch (opcode) {
                case ADD:
                case AND:
                  Rd = (cpu->ir & RD_FIELD) >> RD_FIELD_SHIFT;
                  Rs1 = (cpu->ir & RS1_FIELD) >> RS1_FIELD_SHIFT;
                  if (!(HIGH_ORDER_BIT_VALUE & cpu->ir)){	
                    Rs2 = (cpu->ir & RS2_FIELD) >> RS2_FIELD_SHIFT;
                  } else {
                    immed5 = (cpu->ir & IMMED5_FIELD) >> IMMED5_FIELD_SHIFT;
                    immed5 = sext5(IMMED5_FIELD & cpu->ir);
                    printf("Contents of immed5 = %d\r\n", immed5);
                  }
                  break;
                case NOT:
                  Rd = (cpu->ir & RD_FIELD) >> RD_FIELD_SHIFT;
                  Rs1 = (cpu->ir & RS1_FIELD) >> RS1_FIELD_SHIFT;
                  break;
                case TRAP:
                  trapVector8 = (cpu->ir & TRAP_VECTOR8_FIELD) >> TRAP_VECTOR8_FIELD_SHIFT;
                  break;
                case LD:
                  Rd = (cpu->ir & RD_FIELD) >> RD_FIELD_SHIFT;
                  offset9 = (cpu->ir & OFFSET9_FIELD) >> OFFSET9_FIELD_SHIFT;
                  break;
                case ST:
                  Rs1 = (cpu->ir & RD_FIELD) >> RD_FIELD_SHIFT;
                  offset9 = (cpu->ir & OFFSET9_FIELD) >> OFFSET9_FIELD_SHIFT;
                  break;
                case JMP:
                  BaseR = (cpu->ir & RS1_FIELD) >> RS1_FIELD_SHIFT;
                  break;
                case BR:
                  offset9 = (cpu->ir & OFFSET9_FIELD) >> OFFSET9_FIELD_SHIFT;
                  break;
              }
              BEN = ((cpu->ir & 0x0800) & cpu->n) | ((cpu->ir & 0x0400) & cpu->z) | ((cpu->ir & 0x0200) & cpu->p);  
              state = EVAL_ADDR;
              break;
            case EVAL_ADDR:
              switch (opcode) {
                case LD:
                case ST:
                  cpu->mar = cpu->pc + sext9(OFFSET9_FIELD & cpu->ir);
                  break;
                case TRAP:
                  cpu->mar = TRAP_VECTOR8_FIELD & cpu->ir;
                break;
              }
              state = FETCH_OP;
              break;
            case FETCH_OP:
              switch (opcode) {
                case ADD:
                case AND:
                  if (!(HIGH_ORDER_BIT_VALUE & cpu->ir)) {
                    alu->b = cpu->reg_file[Rs2];
                  } else {
                    alu->b = sext5 (immed5);
                  }
                case NOT:
                  alu->a = cpu->reg_file[Rs1];
                  break;
                case ST:
                  cpu->mdr = Rd;
                  break;
                case LD:
                  cpu->mdr = memory[cpu->mar];
                  break;
                case TRAP:
                  break;
              }
              state = EXECUTE;
              break;
            case EXECUTE:
              switch (opcode) {
                case ADD:
                  alu->r = alu->a + alu->b;
                  setFlags(&cpu, &alu, opcode, Rd);
                  break;
                case AND:
                  alu->r = alu->a & alu->b;
                  setFlags(&cpu, &alu, opcode, Rd);
                  break;
                case NOT:
                  alu->r = ~alu->a;
                  setFlags(&cpu, &alu, opcode, Rd);
                  break;
                case TRAP:
                  trap(cpu->mar);
                  break;
                case BR: 
                  if (BEN) {
                    cpu->pc = cpu->pc + sext9(offset9);
                  }
                  break;
                case JMP:
                  cpu->pc = cpu->reg_file[BaseR];
                  break;
              }
              state = STORE;
              break;
            case STORE:
              switch (opcode) {
                case ADD:
                case AND:
                case NOT:
                  cpu->reg_file[Rd] = alu->r;
                  break;
                case ST:
                  memory[cpu->mar] = cpu->mdr;
                  break;
                case LD:
                  cpu->reg_file[Rd] = cpu->mdr;
                  setFlags(&cpu, &alu, opcode, Rd);
                  break;
                case TRAP:
                  break;
              }
              state = FETCH;
              //display(&cpu, &alu);
              break;
        }
    }
	return 0;
}


// This is our main function that takes a single hex command from the command line
// and runs that in the controller. We use the trap command HALT to stop the program after
// the command runs.
int main (int argc, char* argv[]) {
  int n = 1;
  for (n; n < argc && n < 33; n++) {
    sscanf(argv[n], "%X", &memory[n-1]);
  }
  memory[31] = 0xF019;
	CPU_p cpu = malloc (sizeof(CPU_s));
	ALU_p alu = malloc (sizeof(ALU_s));
  initializeCPU(&cpu, &alu);
  display(&cpu, &alu);
	controller (cpu, alu);
  free(cpu);
  free(alu);
	return 0;
}

void display(CPU_p *cpu, ALU_p *alu) {
  int disp_mem = ((int) memory_start) - 12288;
  printf("\tWelcome to the LC-3 Simulator Simulator\n");
  printf("\tRegisters\t\t\tMemory\n");
  printf("\tR0: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[0], memory_start, memory[disp_mem]);
  printf("\tR1: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[1], memory_start + 1, memory[disp_mem + 1]);
  printf("\tR2: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[2], memory_start + 2, memory[disp_mem + 2]);
  printf("\tR3: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[3], memory_start + 3, memory[disp_mem + 3]);
  printf("\tR4: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[4], memory_start + 4, memory[disp_mem + 4]);
  printf("\tR5: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[5], memory_start + 5, memory[disp_mem + 5]);
  printf("\tR6: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[6], memory_start + 6, memory[disp_mem + 6]);
  printf("\tR7: X%4X\t\t\tX%4X: X%4X\n", (*cpu)->reg_file[7], memory_start + 7, memory[disp_mem + 7]);
  printf("\t\t\t\t\tX%4X: X%4X\n", memory_start + 8, memory[disp_mem + 8]);
  printf("\t\t\t\t\tX%4X: X%4X\n", memory_start + 9, memory[disp_mem + 9]);
  printf("\t\t\t\t\tX%4X: X%4X\n", memory_start + 10, memory[disp_mem + 10]);
  printf("\t PC: X%4i\t IR: X%4X\tX%4X: X%4X\n", (*cpu)->pc + 3000, (*cpu)->ir, memory_start + 11, memory[disp_mem + 11]);
  printf("\t  A: X%4X\t  B: X%4X\tX%4X: X%4X\n", (*alu)->a, (*alu)->b, memory_start + 12, memory[disp_mem + 12]);
  printf("\tMAR: X%4X\tMDR: X%4X\tX%4X: X%4X\n", (*cpu)->mar, (*cpu)->mdr, memory_start + 13, memory[disp_mem + 13]);
  printf("\tCC: N:%i Z:%i P:%i\t\t\tX%4X: X%4X\n", (*cpu)->n, (*cpu)->z, (*cpu)->p, memory_start + 14, memory[disp_mem + 14]);
  printf("\t\t\t\t\tX%4X: X%4X\n", memory_start + 15, memory[disp_mem + 15]);
  printf("Select: 1) Load, 3) Step, 5) Display Mem, 9)Exit\n");
  printf(">_\n");
  printf("-----------------------------------------------------\n");
}

void initializeCPU(CPU_p *cpu, ALU_p *alu) {
  (*cpu)->reg_file[0] = 0;
  (*cpu)->reg_file[1] = 1;
  (*cpu)->reg_file[2] = 2;
  (*cpu)->reg_file[3] = 3;
  (*cpu)->reg_file[4] = 4;
  (*cpu)->reg_file[5] = 5;
  (*cpu)->reg_file[6] = 6;
  (*cpu)->reg_file[7] = 7;
  (*cpu)->pc = 0;
  (*cpu)->ir = 0;
  (*cpu)->mar = 0;
  (*cpu)->mdr = 0;
  (*alu)->a = 0;
  (*alu)->b = 0;
  (*alu)->r = 0;
  (*cpu)->n = 0;
  (*cpu)->z = 0;
  (*cpu)->p = 0;
}

void setFlags(CPU_p *cpu, ALU_p *alu, Register opcode, Register Rd) {
  if(opcode == LD) {                  
    if ((*cpu)->reg_file[Rd] > 0) {
      (*cpu)->n = 0;
      (*cpu)->z = 0;
      (*cpu)->p = 1;
    } else if ((*cpu)->reg_file[Rd] == 0) {
      (*cpu)->n = 0;
      (*cpu)->z = 1;
      (*cpu)->p = 0;
    } else {
      (*cpu)->n = 1;
      (*cpu)->z = 0;
      (*cpu)->p = 0;
    }
  } else {
    if ((*alu)->r > 0) {
      (*cpu)->n = 0;
      (*cpu)->z = 0;
      (*cpu)->p = 1;
    } else if ((*alu)->r == 0) {
      (*cpu)->n = 0;
      (*cpu)->z = 1;
      (*cpu)->p = 0;
    } else {
      (*cpu)->n = 1;
      (*cpu)->z = 0;
      (*cpu)->p = 0;
    }
  }
}


