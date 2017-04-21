/*
	Author: Connor Lundberg, Vincent Povio
	Date: 4/21/2017
	
	The first version of our LC3 personal simulation. It takes a single command
	and runs it through the controller, terminating with a HALT trap. We do this
	to end the loop and also to cut out the HALT trap test.
*/
#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

//current version


// you can define a simple memory module here for this program
unsigned int memory[MAX_MEMORY];   // 32 words of memory enough to store simple program


void initializeCPU(CPU_p *, ALU_p *);
void display(CPU_p *, ALU_p *, int);
void setFlags(CPU_p *, ALU_p *, Register, Register);

// This is the trap function that handles trap vectors. Acts as 
// the trap vector table for now. Currently exits the HALT trap command.
int trap(CPU_p cpu, ALU_p alu, int trap_vector) {

	if (trap_vector == HALT) { //HALT
		initializeCPU(&cpu, &alu);
		return 0;
	}
}


// This is the sext for the immed5. 
int sext5(int immed5) {
	if (HIGH_ORDER_BIT_VALUE & immed5) return (immed5 | SEXT5_SIGN_EXTEND);
	else return immed5;
}


// This is the sext for the PCOffset9.
int sext9(int offset9) {
	if (HIGH_ORDER_BIT_VALUE9 & offset9) return (offset9 | SEXT9_SIGN_EXTEND);
	else return offset9;
}


// This is the main controller for our CPU. It is complete with all microstates
// defined for this project.
int controller (CPU_p cpu, ALU_p alu) {
    // check to make sure both pointers are not NULL
    // do any initializations here
	Register opcode, Rd, Rs1, Rs2, immed5, offset9;	// fields for the IR
	Register effective_addr, trapVector8, BaseR;
	char *nextLine = malloc (sizeof(char));
  initializeCPU(&cpu, &alu);
    int state = FETCH, BEN;
    for (;;) {
        switch (state) {
            case FETCH:
			  display(&cpu, &alu, 0);
			  scanf("%c", nextLine);	//This is for the user to press enter to go to next step.
										//Probably a better way to do this.
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
              BEN = ((cpu->ir & N_CHECK) & cpu->n) | ((cpu->ir & Z_CHECK) & cpu->z) | ((cpu->ir & P_CHECK) & cpu->p);  
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
                  if (!trap(cpu, alu, cpu->mar)) {
					return 0;
				  }
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
	free(nextLine);
	return 0;
}


// This is our main function that takes a single hex command from the command line
// and runs that in the controller. We use the trap command HALT to stop the program after
// the command runs.
int main (int argc, char* argv[]) {
  int n = 1, response = 0;
  unsigned short newAddress;
  FILE *infile;
  char file_name[MAX_MEMORY];
  char *nextLine = malloc(sizeof(char));
  char temp_start[MAX_MEMORY];
	CPU_p cpu = malloc (sizeof(CPU_s));
	ALU_p alu = malloc (sizeof(ALU_s));

  initializeCPU(&cpu, &alu);

  while (response != EXIT) {
    display(&cpu, &alu, 1);
    scanf("%d", &response);
    if (response == LOAD) {
      printf(" File name: ");
      scanf("%s", file_name);
      infile = fopen(file_name, "r");
      if (infile != NULL) {
        while (fscanf(infile, "%X", &memory[n-1]) != EOF && n < MAX_MEMORY) { n++; } ;
      } else {
        printf("ERROR: File not found. Press <ENTER> to continue.");
      }
    } else if (response == STEP) {
	  controller (cpu, alu);
	} else if (response == SHOW_MEM) {
	  printf ("Starting Address: ");
	  scanf("%X", &newAddress);
	  
	  if (newAddress >= START_MEM && newAddress <= (END_MEM - 16)) {
		  
		cpu->memory_start = newAddress;
		display(&cpu, &alu, 1);
	  } else {
		printf ("Not a valid address <ENTER> to continue.");
	  }
	}
  }
	
  free(cpu);
  free(alu);
  fclose(infile);
	return 0;
}


//This is our UI function to display the debug monitor in the console.
//It takes in the CPU, ALU, and an int showChoices that just changes 
//whether the console will display the choices bar at the bottom or not.
//(when using step)
void display(CPU_p *cpu, ALU_p *alu, int showChoices) {
  int disp_mem = ((int) (*cpu)->memory_start) - DISPLAY_HELP;
  if (showChoices) {
	printf("\n\tWelcome to the LC-3 Simulator Simulator\n");
  } else {
	printf("\n\n");
  }
  printf("\tRegisters\t\t\tMemory\n");
  printf("\tR0: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[0], (*cpu)->memory_start, memory[disp_mem]);
  printf("\tR1: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[1], (*cpu)->memory_start + 1, memory[disp_mem + 1]);
  printf("\tR2: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[2], (*cpu)->memory_start + 2, memory[disp_mem + 2]);
  printf("\tR3: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[3], (*cpu)->memory_start + 3, memory[disp_mem + 3]);
  printf("\tR4: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[4], (*cpu)->memory_start + 4, memory[disp_mem + 4]);
  printf("\tR5: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[5], (*cpu)->memory_start + 5, memory[disp_mem + 5]);
  printf("\tR6: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[6], (*cpu)->memory_start + 6, memory[disp_mem + 6]);
  printf("\tR7: X%04X\t\t\tX%04X: X%04X\n", (*cpu)->reg_file[7], (*cpu)->memory_start + 7, memory[disp_mem + 7]);
  printf("\t\t\t\t\tX%04X: X%04X\n", (*cpu)->memory_start + 8, memory[disp_mem + 8]);
  printf("\t\t\t\t\tX%04X: X%04X\n", (*cpu)->memory_start + 9, memory[disp_mem + 9]);
  printf("\t\t\t\t\tX%04X: X%04X\n", (*cpu)->memory_start + 10, memory[disp_mem + 10]);
  printf("\t PC: X%4X\t IR: X%04X\tX%04X: X%04X\n", (*cpu)->pc + START_MEM, (*cpu)->ir, (*cpu)->memory_start + 11, memory[disp_mem + 11]);
  printf("\t  A: X%04X\t  B: X%04X\tX%04X: X%04X\n", (*alu)->a, (*alu)->b, (*cpu)->memory_start + 12, memory[disp_mem + 12]);
  printf("\tMAR: X%04X\tMDR: X%04X\tX%04X: X%04X\n", (*cpu)->mar, (*cpu)->mdr, (*cpu)->memory_start + 13, memory[disp_mem + 13]);
  printf("\tCC: N:%i Z:%i P:%i\t\t\tX%04X: X%04X\n", (*cpu)->n, (*cpu)->z, (*cpu)->p, (*cpu)->memory_start + 14, memory[disp_mem + 14]);
  printf("\t\t\t\t\tX%04X: X%04X\n", (*cpu)->memory_start + 15, memory[disp_mem + 15]);
  if (showChoices) {
	printf("Select: 1) Load, 3) Step, 5) Display Mem, 9)Exit\n");
  }
  printf(">");
}


//Used to get everything set up.
void initializeCPU(CPU_p *cpu, ALU_p *alu) {
  (*cpu)->reg_file[0] = 0;
  (*cpu)->reg_file[1] = 1;
  (*cpu)->reg_file[2] = 2;
  (*cpu)->reg_file[3] = 3;
  (*cpu)->reg_file[4] = 4;
  (*cpu)->reg_file[5] = 5;
  (*cpu)->reg_file[6] = 6;
  (*cpu)->reg_file[7] = 0xA;
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
  (*cpu)->memory_start = START_MEM;
  memory[31] = END;
}


//A function to reduce complexity of controller, just sets the nzp flags
//based on the opcode used.
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


