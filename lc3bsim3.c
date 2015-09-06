
/***************************************************************/
/*                                                             */
/*   LC-3b Simulator - Lab 3                                   */
/*                                                             */
/*   ECE 3056A                                                 */
/*   This is a modified version of the LC-3b Pipeline Design   */
/*   from Prof. Yale Patt's course at UT Austin                */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/
void FETCH_stage();
void DE_stage();
void AGEX_stage();
void MEM_stage();
void SR_stage();
/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define TRUE  1
#define FALSE 0

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
/* control signals from the control store */
enum CS_BITS {
  SR1_NEEDED,
  SR2_NEEDED,
  DRMUX,

  ADDR1MUX,
  ADDR2MUX1, ADDR2MUX0,
  LSHF1,
  ADDRESSMUX,
  SR2MUX,
  ALUK1, ALUK0,
  ALU_RESULTMUX,

  BR_OP,
  UNCOND_OP,
  TRAP_OP,
  BR_STALL,

  DCACHE_EN,
  DCACHE_RW,
  DATA_SIZE,

  DR_VALUEMUX1, DR_VALUEMUX0,
  LD_REG,
  LD_CC,
  NUM_CONTROL_STORE_BITS

} CS_BITS;


enum AGEX_CS_BITS {

  AGEX_ADDR1MUX,
  AGEX_ADDR2MUX1, AGEX_ADDR2MUX0,
  AGEX_LSHF1,
  AGEX_ADDRESSMUX,
  AGEX_SR2MUX,
  AGEX_ALUK1, AGEX_ALUK0,
  AGEX_ALU_RESULTMUX,

  AGEX_BR_OP,
  AGEX_UNCOND_OP,
  AGEX_TRAP_OP,
  AGEX_BR_STALL,
  AGEX_DCACHE_EN,
  AGEX_DCACHE_RW,
  AGEX_DATA_SIZE,

  AGEX_DR_VALUEMUX1, AGEX_DR_VALUEMUX0,
  AGEX_LD_REG,
  AGEX_LD_CC,
  NUM_AGEX_CS_BITS
} AGEX_CS_BITS;

enum MEM_CS_BITS {
  MEM_BR_OP,
  MEM_UNCOND_OP,
  MEM_TRAP_OP,
  MEM_BR_STALL,
  MEM_DCACHE_EN,
  MEM_DCACHE_RW,
  MEM_DATA_SIZE,

  MEM_DR_VALUEMUX1, MEM_DR_VALUEMUX0,
  MEM_LD_REG,
  MEM_LD_CC,
  NUM_MEM_CS_BITS
} MEM_CS_BITS;

enum SR_CS_BITS {
  SR_DR_VALUEMUX1, SR_DR_VALUEMUX0,
  SR_LD_REG,
  SR_LD_CC,
  NUM_SR_CS_BITS
} SR_CS_BITS;


/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int Get_SR1_NEEDED(int *x)     { return (x[SR1_NEEDED]); }
int Get_SR2_NEEDED(int *x)     { return (x[SR2_NEEDED]); }
int Get_DRMUX(int *x)          { return (x[DRMUX]);}
int Get_DE_BR_OP(int *x)       { return (x[BR_OP]); }
int Get_ADDR1MUX(int *x)       { return (x[AGEX_ADDR1MUX]); }
int Get_ADDR2MUX(int *x)       { return ((x[AGEX_ADDR2MUX1] << 1) + x[AGEX_ADDR2MUX0]); }
int Get_LSHF1(int *x)          { return (x[AGEX_LSHF1]); }
int Get_ADDRESSMUX(int *x)     { return (x[AGEX_ADDRESSMUX]); }
int Get_SR2MUX(int *x)          { return (x[AGEX_SR2MUX]); }
int Get_ALUK(int *x)           { return ((x[AGEX_ALUK1] << 1) + x[AGEX_ALUK0]); }
int Get_ALU_RESULTMUX(int *x)  { return (x[AGEX_ALU_RESULTMUX]); }
int Get_BR_OP(int *x)          { return (x[MEM_BR_OP]); }
int Get_UNCOND_OP(int *x)      { return (x[MEM_UNCOND_OP]); }
int Get_TRAP_OP(int *x)        { return (x[MEM_TRAP_OP]); }
int Get_DCACHE_EN(int *x)      { return (x[MEM_DCACHE_EN]); }
int Get_DCACHE_RW(int *x)      { return (x[MEM_DCACHE_RW]); }
int Get_DATA_SIZE(int *x)      { return (x[MEM_DATA_SIZE]); }
int Get_DR_VALUEMUX1(int *x)   { return ((x[SR_DR_VALUEMUX1] << 1 ) + x[SR_DR_VALUEMUX0]); }
int Get_AGEX_LD_REG(int *x)    { return (x[AGEX_LD_REG]); }
int Get_AGEX_LD_CC(int *x)     { return (x[AGEX_LD_CC]); }
int Get_MEM_LD_REG(int *x)     { return (x[MEM_LD_REG]); }
int Get_MEM_LD_CC(int *x)      { return (x[MEM_LD_CC]); }
int Get_SR_LD_REG(int *x)      { return (x[SR_LD_REG]); }
int Get_SR_LD_CC(int *x)       { return (x[SR_LD_CC]); }
int Get_DE_BR_STALL(int *x)    { return (x[BR_STALL]); }
int Get_AGEX_BR_STALL(int *x)  { return (x[AGEX_BR_STALL]); }
int Get_MEM_BR_STALL(int *x)   { return (x[MEM_BR_STALL]); }



/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][NUM_CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A
   There are two write enable signals, one for each byte. WE0 is used for
   the least significant byte of a word. WE1 is used for the most significant
   byte of a word. */

#define WORDS_IN_MEM    0x08000
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/
/* The LC-3b register file.                                      */
/***************************************************************/
#define LC3b_REGS 8
int REGS[8] = {0,1,2,3,4,5,0x123a,0xABCD};
/***************************************************************/
/* architectural state */
/***************************************************************/
int  PC,  	/* program counter */
     N,		/* n condition bit */
     Z = 1,	/* z condition bit */
     P;		/* p condition bit */
/***************************************************************/
/* LC-3b State info.                                             */
/***************************************************************/

typedef struct PipeState_Entry_Struct{

  /* DE latches */
int DE_NPC,
    DE_IR,
    DE_V,
    /* AGEX lateches */
    AGEX_NPC,
    AGEX_SR1,
    AGEX_SR2,
    AGEX_CC,
    AGEX_IR,
    AGEX_DRID,
    AGEX_V,
    AGEX_CS[NUM_AGEX_CS_BITS],
    /* MEM latches */
    MEM_NPC,
    MEM_ALU_RESULT,
    MEM_ADDRESS,
    MEM_CC,
    MEM_IR,
    MEM_DRID,
    MEM_V,
    MEM_CS[NUM_MEM_CS_BITS],
    /* SR latches */
    SR_NPC,
    SR_DATA,
    SR_ALU_RESULT,
    SR_ADDRESS,
    SR_IR,
    SR_DRID,
    SR_V,
    SR_CS[NUM_SR_CS_BITS];

} PipeState_Entry;

/* data structure for latch */
PipeState_Entry PS, NEW_PS;

/* simulator signal */
int RUN_BIT;

/* Internal stall signals */
int   dep_stall,
      v_de_br_stall,
      v_agex_br_stall,
      v_mem_br_stall,
      mem_stall,
      icache_r;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the architectural state    \n");
    printf("idump            -  dump the internal state         \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

void print_CS(int *CS, int num)
{
  int ii ;
  for ( ii = 0 ; ii < num; ii++) {
    printf("%d",CS[ii]);
  }
  printf("\n");
}
/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {
  NEW_PS = PS;
  SR_stage();
  MEM_stage();
  AGEX_stage();
  DE_stage();
  FETCH_stage();
  PS = NEW_PS;
  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (RUN_BIT == FALSE) {
      printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (PC == 0x0000) {
	    cycle();
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {
    if ((RUN_BIT == FALSE) || (PC == 0x0000)) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }
    printf("Simulating...\n\n");
    /* initialization */
    while (PC != 0x0000)
      cycle();
    cycle();
      RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a region of memory to the output file.     */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current architectural state  to the       */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count : %d\n", CYCLE_COUNT);
    printf("PC          : 0x%04x\n", PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    printf("Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	printf("%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent architectural state :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC          : 0x%04x\n", PC);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : idump                                           */
/*                                                             */
/* Purpose   : Dump current internal state to the              */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void idump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count     : %d\n", CYCLE_COUNT);
    printf("PC              : 0x%04x\n", PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    printf("Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	printf("%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    printf("\n");

    printf("------------- Stall Signals -------------\n");
    printf("ICACHE_R        :  %d\n", icache_r);
    printf("DEP_STALL       :  %d\n", dep_stall);
    printf("V_DE_BR_STALL   :  %d\n", v_de_br_stall);
    printf("V_AGEX_BR_STALL :  %d\n", v_agex_br_stall);
    printf("MEM_STALL       :  %d\n", mem_stall);
    printf("V_MEM_BR_STALL  :  %d\n", v_mem_br_stall);
    printf("\n");

    printf("------------- DE   Latches --------------\n");
    printf("DE_NPC          :  0x%04x\n", PS.DE_NPC );
    printf("DE_IR           :  0x%04x\n", PS.DE_IR );
    printf("DE_V            :  %d\n", PS.DE_V);
    printf("\n");

    printf("------------- AGEX Latches --------------\n");
    printf("AGEX_NPC        :  0x%04x\n", PS.AGEX_NPC );
    printf("AGEX_SR1        :  0x%04x\n", PS.AGEX_SR1 );
    printf("AGEX_SR2        :  0x%04x\n", PS.AGEX_SR2 );
    printf("AGEX_CC         :  %d\n", PS.AGEX_CC );
    printf("AGEX_IR         :  0x%04x\n", PS.AGEX_IR );
    printf("AGEX_DRID       :  %d\n", PS.AGEX_DRID);
    printf("AGEX_CS         :  ");
    for ( k = 0 ; k < NUM_AGEX_CS_BITS; k++) {
      printf("%d",PS.AGEX_CS[k]);
    }
    printf("\n");
    printf("AGEX_V          :  %d\n", PS.AGEX_V);
    printf("\n");

    printf("------------- MEM  Latches --------------\n");
    printf("MEM_NPC         :  0x%04x\n", PS.MEM_NPC );
    printf("MEM_ALU_RESULT  :  0x%04x\n", PS.MEM_ALU_RESULT );
    printf("MEM_ADDRESS     :  0x%04x\n", PS.MEM_ADDRESS );
    printf("MEM_CC          :  %d\n", PS.MEM_CC );
    printf("MEM_IR          :  0x%04x\n", PS.MEM_IR );
    printf("MEM_DRID        :  %d\n", PS.MEM_DRID);
    printf("MEM_CS          :  ");
    for ( k = 0 ; k < NUM_MEM_CS_BITS; k++) {
      printf("%d",PS.MEM_CS[k]);
    }
    printf("\n");
    printf("MEM_V           :  %d\n", PS.MEM_V);
    printf("\n");

    printf("------------- SR   Latches --------------\n");
    printf("SR_NPC          :  0x%04x\n", PS.SR_NPC );
    printf("SR_DATA         :  0x%04x\n", PS.SR_DATA );
    printf("SR_ALU_RESULT   :  0x%04x\n", PS.SR_ALU_RESULT );
    printf("SR_ADDRESS      :  0x%04x\n", PS.SR_ADDRESS );
    printf("SR_IR           :  0x%04x\n", PS.SR_IR );
    printf("SR_DRID         :  %d\n", PS.SR_DRID);
    printf("SR_CS           :  ");
    for ( k = 0 ; k < NUM_SR_CS_BITS; k++) {
      printf("%d",PS.SR_CS[k]);
    }
    printf("\n");
    printf("SR_V            :  %d\n", PS.SR_V);

    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file,"\nCurrent architectural state :\n");
    fprintf(dumpsim_file,"-------------------------------------\n");
    fprintf(dumpsim_file,"Cycle Count     : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file,"PC              : 0x%04x\n", PC);
    fprintf(dumpsim_file,"CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    fprintf(dumpsim_file,"Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
      fprintf(dumpsim_file,"%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- Stall Signals -------------\n");
    fprintf(dumpsim_file,"ICACHE_R        :  %d\n", icache_r);
    fprintf(dumpsim_file,"DEP_STALL       :  %d\n", dep_stall);
    fprintf(dumpsim_file,"V_DE_BR_STALL   :  %d\n", v_de_br_stall);
    fprintf(dumpsim_file,"V_AGEX_BR_STALL :  %d\n", v_agex_br_stall);
    fprintf(dumpsim_file,"MEM_STALL       :  %d\n", mem_stall);
    fprintf(dumpsim_file,"V_MEM_BR_STALL  :  %d\n", v_mem_br_stall);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- DE   Latches --------------\n");
    fprintf(dumpsim_file,"DE_NPC          :  0x%04x\n", PS.DE_NPC );
    fprintf(dumpsim_file,"DE_IR           :  0x%04x\n", PS.DE_IR );
    fprintf(dumpsim_file,"DE_V            :  %d\n", PS.DE_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- AGEX Latches --------------\n");
    fprintf(dumpsim_file,"AGEX_NPC        :  0x%04x\n", PS.AGEX_NPC );
    fprintf(dumpsim_file,"AGEX_SR1        :  0x%04x\n", PS.AGEX_SR1 );
    fprintf(dumpsim_file,"AGEX_SR2        :  0x%04x\n", PS.AGEX_SR2 );
    fprintf(dumpsim_file,"AGEX_CC         :  %d\n", PS.AGEX_CC );
    fprintf(dumpsim_file,"AGEX_IR         :  0x%04x\n", PS.AGEX_IR );
    fprintf(dumpsim_file,"AGEX_DRID       :  %d\n", PS.AGEX_DRID);
    fprintf(dumpsim_file,"AGEX_CS         :  ");
    for ( k = 0 ; k < NUM_AGEX_CS_BITS; k++) {
      fprintf(dumpsim_file,"%d",PS.AGEX_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"AGEX_V          :  %d\n", PS.AGEX_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- MEM  Latches --------------\n");
    fprintf(dumpsim_file,"MEM_NPC         :  0x%04x\n", PS.MEM_NPC );
    fprintf(dumpsim_file,"MEM_ALU_RESULT  :  0x%04x\n", PS.MEM_ALU_RESULT );
    fprintf(dumpsim_file,"MEM_ADDRESS     :  0x%04x\n", PS.MEM_ADDRESS );
    fprintf(dumpsim_file,"MEM_CC          :  %d\n", PS.MEM_CC );
    fprintf(dumpsim_file,"MEM_IR          :  0x%04x\n", PS.MEM_IR );
    fprintf(dumpsim_file,"MEM_DRID        :  %d\n", PS.MEM_DRID);
    fprintf(dumpsim_file,"MEM_CS          :  ");
    for ( k = 0 ; k < NUM_MEM_CS_BITS; k++) {
      fprintf(dumpsim_file,"%d",PS.MEM_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"MEM_V           :  %d\n", PS.MEM_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- SR   Latches --------------\n");
    fprintf(dumpsim_file,"SR_NPC          :  0x%04x\n", PS.SR_NPC );
    fprintf(dumpsim_file,"SR_DATA         :  0x%04x\n",PS.SR_DATA );
    fprintf(dumpsim_file,"SR_ALU_RESULT   :  0x%04x\n", PS.SR_ALU_RESULT );
    fprintf(dumpsim_file,"SR_ADDRESS      :  0x%04x\n", PS.SR_ADDRESS );
    fprintf(dumpsim_file,"SR_IR           :  0x%04x\n", PS.SR_IR );
    fprintf(dumpsim_file,"SR_DRID         :  %d\n", PS.SR_DRID);
    fprintf(dumpsim_file,"SR_CS           :  ");
    for ( k = 0 ; k < NUM_SR_CS_BITS; k++) {
    fprintf(dumpsim_file, "%d",PS.SR_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"SR_V            :  %d\n", PS.SR_V);

    fprintf(dumpsim_file,"\n");


}
/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    case 'I':
    case 'i':
        idump(dumpsim_file);
        break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < NUM_CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}
	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory() {
    int i;

     for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}


/***************************************************************/
/*                                                             */
/* Procedure : init_state                                      */
/*                                                             */
/* Purpose   : Zero out all latches and registers              */
/*                                                             */
/***************************************************************/
void init_state() {

  memset(&PS, 0 ,sizeof(PipeState_Entry));
  memset(&NEW_PS, 0 , sizeof(PipeState_Entry));

  dep_stall       = 0;
  v_de_br_stall   = 0;
  v_agex_br_stall = 0;
  v_mem_br_stall  = 0;
  mem_stall       = 0;
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1 ;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (PC == 0) PC  = program_base << 1 ;
    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) {
    int i;
    init_control_store(ucode_filename);

    init_memory();

    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    init_state();

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* dcache_access                                               */
/*                                                             */
/***************************************************************/
void dcache_access(int dcache_addr, int *read_word, int write_word, int *dcache_r,
		    int mem_w0, int mem_w1) {

  int addr = dcache_addr >> 1 ;
  int random = CYCLE_COUNT % 9;

  if (!random) {
    *dcache_r = 0;
    *read_word = 0xfeed ;
  }
  else {
    *dcache_r = 1;

    *read_word = (MEMORY[addr][1] << 8) | (MEMORY[addr][0] & 0x00FF);
    if(mem_w0) MEMORY[addr][0] = write_word & 0x00FF;
    if(mem_w1) MEMORY[addr][1] = (write_word & 0xFF00) >> 8;
  }
}
/***************************************************************/
/*                                                             */
/* icache_access                                               */
/*                                                             */
/***************************************************************/
void icache_access(int icache_addr, int *read_word, int *icache_r) {

  int addr = icache_addr >> 1 ;
  int random = CYCLE_COUNT % 13;

  if (!random) {
    *icache_r = 0;
    *read_word = 0xfeed;
  }
  else {
    *icache_r = 1;
    *read_word = MEMORY[addr][1] << 8 | MEMORY[addr][0];
  }
}
/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);
}


/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   RUN_BIT
   REGS
   MEMORY

   PC
   N
   Z
   P

   dep_stall
   v_de_br_stall
   v_agex_br_stall
   v_mem_br_stall
   mem_stall
   icache_r

   PS
   NEW_PS


   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.


   Begin your code here 	  			       */
/***************************************************************/
#define COPY_AGEX_CS_START 3
#define COPY_MEM_CS_START 9
#define COPY_SR_CS_START  7

/*Functions from Lab1*/
int get_bits(int value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}


int read_word(int addr){
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][1]<<8 | MEMORY[addr>>1][0];
}

int read_byte(int addr){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][bank];
}

void write_byte(int addr, int value){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][bank]= value & 0xFF;
}

void write_word(int addr, int value){
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][1] = (value & 0x0000FF00) >> 8;
  MEMORY[addr>>1][0] = value & 0xFF;
}



int LSHF(int value, int amount){
  return (value << amount) & 0xFFFF;
}

int RSHF(int value, int amount, int topbit ){
  int mask;
  mask = 1 << amount;
  mask -= 1;
  mask = mask << ( 16 -amount );

  return ((value >> amount) & ~mask) | ((topbit)?(mask):0); /* TBD */
}

int SEXT(int value, int topbit){
  int shift = sizeof(int)*8 - topbit;
  return (value << shift )>> shift;
}

int ZEXT(int value, int topbit){
	return (value & ( (1<<(topbit+1))-1) );
}










int ALU(int A, int B, int ALUK){
	int retval;

	switch(ALUK){
		case 0: retval = A+B;
		break;

		case 1: retval = A&B;
		break;

		case 2: retval = A^B;
		break;

		case 3: retval = B;
		break;

		default: exit(-1);
	}

	return Low16bits(retval);

}
int SHF(){
	int Gate_SHF;
	if(get_bits(PS.AGEX_IR,4,4)==0){
		Gate_SHF=LSHF(PS.AGEX_SR1,get_bits(PS.AGEX_IR,3,0));
		return Low16bits(Gate_SHF);
	}else{
		if(get_bits(PS.AGEX_IR,5,5)==0){
			Gate_SHF=RSHF(PS.AGEX_SR1,get_bits(PS.AGEX_IR,3,0),0);
			return Low16bits(Gate_SHF);
		}else{

			Gate_SHF=RSHF(PS.AGEX_SR1,get_bits(PS.AGEX_IR,3,0),get_bits(PS.AGEX_SR1,15,15));
			return Low16bits(Gate_SHF);
		}

	}


}


/*Functions from recitation*/



/* Signals generated by SR stage and needed by previous stages in the
   pipeline are declared below. */
int sr_reg_data,
    sr_n, sr_z, sr_p,
    v_sr_ld_cc,
    v_sr_ld_reg,
    sr_reg_id;


/************************* SR_stage() *************************/

void SR_stage() {
    printf("***************SR***************\n");
  /* You are given the code for SR_stage to get you started. Look at
     the figure for SR stage to see how this code is implemented. */

  switch (Get_DR_VALUEMUX1(PS.SR_CS))
  {
  case 0:
    sr_reg_data = PS.SR_ADDRESS ;
    break;
  case 1:
    sr_reg_data = PS.SR_DATA ;
    break;
  case 2:
    sr_reg_data = PS.SR_NPC ;
    break;
  case 3:
    sr_reg_data = PS.SR_ALU_RESULT ;
    break;
  }

  sr_reg_id = PS.SR_DRID;
  v_sr_ld_reg = Get_SR_LD_REG(PS.SR_CS) & PS.SR_V;
  v_sr_ld_cc = Get_SR_LD_CC(PS.SR_CS) & PS.SR_V ;

  /* CC LOGIC  */
  sr_n = ((sr_reg_data & 0x8000) ? 1 : 0);
  sr_z = ((sr_reg_data & 0xFFFF) ? 0 : 1);
  sr_p = 0;
  if ((!sr_n) && (!sr_z))
    sr_p = 1;

}


/************************* MEM_stage() *************************/
int Get_MEMPCMUX(){
    if(PS.MEM_V & Get_TRAP_OP(PS.MEM_CS)){
        return 2;
    }else if(PS.MEM_V & Get_UNCOND_OP(PS.MEM_CS)){
        return 1;
    }else if((PS.MEM_V) & (Get_BR_OP(PS.MEM_CS))){
        if(get_bits(PS.MEM_IR,11,9)& PS.MEM_CC){
          return 1;
        }
        else{
          return 0;
        }
    }else{
        return 0;
    }
}

int byteOrWord(){

        if(Get_DATA_SIZE(PS.MEM_CS)){
            return Low16bits(PS.MEM_ALU_RESULT);
        }else{
            if(get_bits(PS.MEM_ADDRESS,0,0)){
                return Low16bits(LSHF(get_bits(Low16bits(PS.MEM_ALU_RESULT),7,0),8));
            }else{
                return Low16bits(get_bits(Low16bits(PS.MEM_ALU_RESULT),7,0));
            }

        }


}

int readData(read){
    if(Get_DATA_SIZE(PS.MEM_CS)){
        return Low16bits(read);
    }else{
        if(get_bits(PS.MEM_ADDRESS,0,0)){
                return Low16bits((SEXT(get_bits(read,15,8),8)));
        }else{
                return Low16bits((SEXT(get_bits(read,7,0),8)));
        }

    }

}


int TRAP_PC;
int TARGET_PC;
int MEM_PCMUX;
int mem_reg_id;
int LD_REG_mem;
int LD_CC_mem;
void MEM_stage() {
    printf("***************MEM******************\n");
  int ii,jj = 0;
    int dcache_r,write, read;
  /* your code for MEM stage goes here */
    int size=Get_DATA_SIZE(PS.MEM_CS);
    write=byteOrWord();
    printf("write %x\n",write);
    if(PS.MEM_ADDRESS==0x3024){
        printf("SR.DATA  %x",PS.MEM_ALU_RESULT);
    }

    int we1=(Get_DCACHE_RW(PS.MEM_CS)) & ((get_bits(PS.MEM_ADDRESS,0,0))^size);
    int we0=(Get_DCACHE_RW(PS.MEM_CS)) & (!(get_bits(PS.MEM_ADDRESS,0,0)));
    printf("MEM_ADDRESS %x\n", PS.MEM_ADDRESS);
    if(Get_DCACHE_EN(PS.MEM_CS)&PS.MEM_V){

      /*  if((PS.MEM_ADDRESS==0x3024) && (PS.MEM_ALU_RESULT==0xabcd)){
            printf("cycle %d", CYCLE_COUNT);
            printf("SR.DATA  %x %x",PS.MEM_ALU_RESULT, write);
            exit(-1);
        }*/
        dcache_access(Low16bits(PS.MEM_ADDRESS), &read, write, &dcache_r,we0,we1);

        printf("went mem d_cache\n");

    }
    printf("read %x\n",read);
    read=readData(read);
	TRAP_PC=read;

    TARGET_PC=PS.MEM_ADDRESS;
    printf("TargetPC %x",TARGET_PC);
    if (PS.MEM_V){
       v_mem_br_stall=Get_MEM_BR_STALL(PS.MEM_CS);
       LD_REG_mem=Get_MEM_LD_REG(PS.MEM_CS);
       LD_CC_mem=Get_MEM_LD_CC(PS.MEM_CS);

    }else{
       v_mem_br_stall=0;
       LD_REG_mem=0;
       LD_CC_mem=0;

    }

    mem_reg_id=PS.MEM_DRID;
    printf("dcache_r %d", dcache_r);
    mem_stall=(Get_DCACHE_EN(PS.MEM_CS))&(!dcache_r);

    MEM_PCMUX=Get_MEMPCMUX();
     int valid;
     if ((Get_DCACHE_EN(PS.MEM_CS)|!(dcache_r & Get_DCACHE_EN(PS.MEM_CS)))& !PS.MEM_V){
        valid=0;
     }else{
        valid=1;
     }

     printf("read %x\n",read);

     NEW_PS.SR_DATA=read;


  /* The code below propagates the control signals from MEM.CS latch
     to SR.CS latch. You still need to latch other values into the
     other SR latches. */
     NEW_PS.SR_ALU_RESULT=PS.MEM_ALU_RESULT;
     NEW_PS.SR_IR=PS.MEM_IR;
     NEW_PS.SR_NPC=PS.MEM_NPC;
     NEW_PS.SR_DRID=PS.MEM_DRID;
     NEW_PS.SR_ADDRESS=PS.MEM_ADDRESS;

     NEW_PS.SR_V=PS.MEM_V & (((Get_DCACHE_EN(PS.MEM_CS))&dcache_r) | (!Get_DCACHE_EN(PS.MEM_CS)));

  for (ii=COPY_SR_CS_START; ii < NUM_MEM_CS_BITS; ii++) {
    NEW_PS.SR_CS [jj++] = PS.MEM_CS [ii];
  }

}


/*********************AGEX_stage() *************************/
int Get_SR2(){
    if(Get_SR2MUX(PS.AGEX_CS)){
        return Low16bits(SEXT(get_bits(PS.AGEX_IR,4,0),5));
    }else{
        return Low16bits(PS.AGEX_SR2);
    }

}
int getADDR2MUX(){
	int temp;
	switch (Get_ADDR2MUX(PS.AGEX_CS)){
		case 0:
			temp= 0;
			break;
		case 1:
			temp= SEXT(get_bits(PS.AGEX_IR,5,0),6);
			break;
		case 2:
			temp= SEXT(get_bits(PS.AGEX_IR,8,0),9);
			break;
		case 3:
			temp= SEXT(get_bits(PS.AGEX_IR,10,0),11);
			break;
	}
	return Low16bits(temp);
}

int Get_ADDMUX1(){
    if(Get_ADDR1MUX(PS.AGEX_CS)){
        return Low16bits(PS.AGEX_SR1);
    }else{
        return Low16bits(PS.AGEX_NPC);
    }
}

int Get_ADDER(){
    if(Get_LSHF1(PS.AGEX_CS)){
        return Low16bits(Get_ADDMUX1()+LSHF(getADDR2MUX(),1));

    }else{
        return Low16bits(Get_ADDMUX1()+getADDR2MUX());

    }


}

int agex_reg_id;
int LD_REG_agex;
int LD_CC_agex;

void AGEX_stage() {
    printf("*******************AGEX*************************\n");
  int ii, jj = 0;
  int LD_MEM; /* You need to write code to compute the value of LD.MEM
		 signal */

    if (mem_stall){
        LD_MEM=0;
    }else{
        LD_MEM=1;
    }
    printf("hi LD_MEM %d\n",LD_MEM);
    int MEM_ALU_RES;
    printf("Get_ALU %d\n", Get_ALU_RESULTMUX(PS.AGEX_CS));
  /* your code for AGEX stage goes here */
    if(Get_ALU_RESULTMUX(PS.AGEX_CS)){
        printf("SR1: %d \n SR2: %d \n ", PS.AGEX_SR1,Get_SR2());
        MEM_ALU_RES=ALU(Low16bits(PS.AGEX_SR1),Get_SR2(),Get_ALUK(PS.AGEX_CS));
        printf("hi MEM_ALU %d\n", MEM_ALU_RES);
    }else{
        MEM_ALU_RES=SHF();
    }
    if(PS.AGEX_V){
        v_agex_br_stall=Get_AGEX_BR_STALL(PS.AGEX_CS);
        LD_CC_agex=Get_AGEX_LD_CC(PS.AGEX_CS);
        LD_REG_agex=Get_AGEX_LD_REG(PS.AGEX_CS);

    }else{
        v_agex_br_stall=0;
        LD_CC_agex=0;
        LD_REG_agex=0;

    }

    agex_reg_id=PS.AGEX_DRID;

    int MEM_ADDRESS;
    if(Get_ADDRESSMUX(PS.AGEX_CS)){
        MEM_ADDRESS=Get_ADDER();
    }else{
        MEM_ADDRESS=LSHF(ZEXT(get_bits(PS.AGEX_IR,7,0),8),1);
    }

  if (LD_MEM) {
    /* Your code for latching into MEM latches goes here */
    NEW_PS.MEM_NPC=PS.AGEX_NPC;
    NEW_PS.MEM_ALU_RESULT=MEM_ALU_RES;
    NEW_PS.MEM_IR=PS.AGEX_IR;
    NEW_PS.MEM_DRID=PS.AGEX_DRID;
    NEW_PS.MEM_CC=PS.AGEX_CC;
    NEW_PS.MEM_ADDRESS=MEM_ADDRESS;
    NEW_PS.MEM_V=PS.AGEX_V;
    printf("Sr1 %x\n",PS.AGEX_SR1);
    printf("hi MEM_ALU_RES %x\n",MEM_ALU_RES);


    /* The code below propagates the control signals from AGEX.CS latch
       to MEM.CS latch. */
    for (ii = COPY_MEM_CS_START; ii < NUM_AGEX_CS_BITS; ii++) {
      NEW_PS.MEM_CS [jj++] = PS.AGEX_CS [ii];
    }
  }
}



/************************* DE_stage() *************************/

void DE_stage() {
    printf("******************************DE***************************\n");
  int CONTROL_STORE_ADDRESS;  /* You need to implement the logic to
			         set the value of this variable. Look
			         at the figure for DE stage */
  int ii, jj = 0;

  CONTROL_STORE_ADDRESS=((PS.DE_IR & 0xf800)>>10) | ((PS.DE_IR & 0x0020)>>5);

  int LD_AGEX; /* You need to write code to compute the value of
		  LD.AGEX signal */
	if(mem_stall){
        LD_AGEX=0;
	}else{
        LD_AGEX=1;
	}
  /* your code for DE stage goes here */
    printf("Control Store %d",CONTROL_STORE_ADDRESS);
    printf("GetSr1 signal %d\n",Get_SR1_NEEDED(CONTROL_STORE[CONTROL_STORE_ADDRESS]));

	int SR1;
	int SR2;
	int de_stall1=0;
	int de_stall2=0;
	int de_stall3=0;
	int count=0;
    dep_stall=0;
	int DR;
	if(Get_DRMUX(CONTROL_STORE[CONTROL_STORE_ADDRESS])){
		DR=7;
	}else{
		DR=get_bits(PS.DE_IR,11,9);
	}

    int de_br_stall=LD_CC_mem|v_sr_ld_cc|LD_CC_agex;

    printf("de_br_stall after branch %x\n",de_br_stall);

    SR1=get_bits(PS.DE_IR,8,6);
	int sr1_dep=(LD_REG_mem && (mem_reg_id==SR1)) | (LD_REG_agex && (agex_reg_id==SR1)) | ((sr_reg_id==SR1) && v_sr_ld_reg);
    printf("sr1_dep %d\n",sr1_dep);
    printf("valid DE %d\n",PS.DE_V);

    int SR2_dep=get_bits(PS.DE_IR,13,13);

    if(SR2_dep){
        SR2=get_bits(PS.DE_IR,11,9);
    }else{
        SR2=get_bits(PS.DE_IR,2,0);
    }

    printf("dest reg %d\n", agex_reg_id);
    printf("dest reg %d\n", mem_reg_id);
    printf("dest reg %d\n", sr_reg_id);
    int sr2_dep=(LD_REG_mem & (mem_reg_id==SR2)) | (LD_REG_agex & (agex_reg_id==SR2)) | ((sr_reg_id==SR2) & v_sr_ld_reg);

        if(Get_SR2_NEEDED(CONTROL_STORE[CONTROL_STORE_ADDRESS])){

              de_stall1 = sr2_dep;
              count++;
        }
        if(Get_DE_BR_OP(CONTROL_STORE[CONTROL_STORE_ADDRESS])){

                de_stall2=de_br_stall;
                count++;

        }
        if(Get_SR1_NEEDED(CONTROL_STORE[CONTROL_STORE_ADDRESS])){
            de_stall3=sr1_dep;

        }

    if(PS.DE_V){
        dep_stall=de_stall1|de_stall2|de_stall3;

    }

    printf("de_stall after SR1 %d \n",dep_stall);
    printf("de_stall after SR2 %d\n",dep_stall);
    printf("count %d\n",count);

    int valid;
    if(PS.DE_V & (!dep_stall)){
        valid=1;
    }else{
        valid=0;
    }
    printf("LD_AGEX %d\n",LD_AGEX);
    printf("valid bit %d\n", valid);

    printf("de_v: %d\n br_stall: %d\n", PS.DE_V,Get_DE_BR_STALL(CONTROL_STORE[CONTROL_STORE_ADDRESS]));
    if(PS.DE_V & Get_DE_BR_STALL(CONTROL_STORE[CONTROL_STORE_ADDRESS])) {
        v_de_br_stall = 1;
      } else {
        v_de_br_stall = 0;
      }



  if (LD_AGEX) {
    /* Your code for latching into AGEX latches goes here */
    int set_cc=(N<<2)|(Z<<1)|P;
    NEW_PS.AGEX_SR1=REGS[SR1];
    printf("reg1 : %d\n", REGS[SR1]);
	NEW_PS.AGEX_SR2=REGS[SR2];
    printf("DR %d\n", DR);
	NEW_PS.AGEX_CC=set_cc;
	NEW_PS.AGEX_DRID=DR;
	NEW_PS.AGEX_IR=PS.DE_IR;
	NEW_PS.AGEX_NPC=PS.DE_NPC;
	if(dep_stall){
        NEW_PS.AGEX_V=0;
    }else{
        NEW_PS.AGEX_V=valid;
    }


    /* The code below propagates the control signals from the CONTROL
       STORE to the AGEX.CS latch. */
    for (ii = COPY_AGEX_CS_START; ii< NUM_CONTROL_STORE_BITS; ii++) {
      NEW_PS.AGEX_CS[jj++] = CONTROL_STORE[CONTROL_STORE_ADDRESS][ii];
    }


  }

     if(v_sr_ld_reg & PS.SR_V & icache_r){
        N=sr_n;
        Z=sr_z;
        P=sr_p;
     }
      printf("N %d\n Z %d\n P %d\n",N,Z,P);

    if(v_sr_ld_reg & PS.SR_V){
        REGS[sr_reg_id]=sr_reg_data;
    }


}



/************************* FETCH_stage() *************************/
int count_fetch=0;
void FETCH_stage() {
    printf("********************************FETCH**************************\n");
  /* your code for FETCH stage goes here */
	int LD_PC;
	int LD_DE;
	int IR_FETCH;

	printf("icacher %d",icache_r);

    icache_access(PC,&IR_FETCH,&icache_r);

	printf("PC: %d \n", PC);
	/* if(TESTING){
		// LD_PC=1;
		// LD_DE=1;
	// }*/
	int stall=v_de_br_stall|v_mem_br_stall|v_agex_br_stall;

	if( (( !(v_de_br_stall | v_agex_br_stall) & icache_r ) | (v_mem_br_stall))&(!dep_stall)&(!mem_stall) ){
		LD_PC=1;
	}else{
		LD_PC=0;
	}


	if ((dep_stall | mem_stall)){
		LD_DE=0;
	}else{
		LD_DE=1;
	}

	int valid;
    printf("stall %d",stall);
	if((!stall) & icache_r){
		valid=1;
	}else{
		valid=0;
	}
	printf("inst %x \n", IR_FETCH);
	printf("LD_DE %d\n",LD_DE);
	printf("valid %d\n",valid);
	if(LD_DE){
		NEW_PS.DE_NPC=PC+2;
		NEW_PS.DE_IR=IR_FETCH;
		NEW_PS.DE_V=valid;

	}
    printf("LD_PC %d\n",LD_PC);
    printf("MEM_PCMUX %d\n",MEM_PCMUX);
    if(LD_PC){
        switch(MEM_PCMUX){
            case 0:
                if((!v_de_br_stall)&&(!v_mem_br_stall)){
                   PC=PC+2;
                }
                break;
            case 1:
                PC=TARGET_PC;
                break;
            case 2:
                PC=TRAP_PC;
                break;
            default:
                break;
        }
	}
	printf("NPC: %d \n",NEW_PS.DE_NPC);
}
