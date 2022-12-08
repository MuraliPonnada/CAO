#ifndef APEXCPU_H // Guard against recursive includes
#define APEXCPU_H
#include <stdarg.h> // To enable reportStage

enum fu_enum {
	alu,
	mul,
	ldr,
	str,
	brz
};

enum stageStatus_enum {
	stage_squashed,
	stage_stalled,
	stage_noAction,
	stage_actionComplete
};

struct apexStage_struct {
	int pc;
	int instruction;
	int opcode;
	char mnemonic[8];
	int dr;
	int sr1;
	int sr2;
	int imm;
	int offset;
	int op1;
	int op2;
	int result;
	int effectiveAddr;
	int squashed;
	int stalled;
	char report[128];
	enum stageStatus_enum status;
	int branch_taken;
	enum fu_enum func;
	int pipearr[5];
};

struct CC_struct {
		int z;
		int p;
	};

struct fwdBus_struct {
	int tag;
	int valid;
	int value;
};

struct apexCPU_struct {
	int pc;
	int reg[16];
	int regValid[16];
	struct CC_struct cc;
	struct apexStage_struct stage[18];
	int codeMem[128]; // addresses 0x4000 - 0x4200
	int dataMem[128]; // addresses 0x0000 - 0x0200
	int lowMem;
	int highMem;
	int t;
	int numInstructions;
	int instr_retired;
	int halt_fetch;
	int stop;
	char abend[64];
	struct fwdBus_struct ex_fwdBus,mem_fwdBus;
	int pipearr[5];
};
typedef struct apexCPU_struct * cpu;

enum stage_enum {
	fetch,
	decode,
	alu1,
	alu2,
	alu3,
	mul1,
	mul2,
	mul3,
	ldr1,
	ldr2,
	ldr3,
	str1,
	str2,
	str3,
	brz1,
	brz2,
	brz3,
	writeback
};

extern char *stageName[18]; // defined/initialized in apexCPU.c
typedef void (*opStageFn)(cpu cpu); // Needed in apexOpcodes.h

#include "apexOpcodes.h"


void initCPU(cpu cpu);
void loadCPU(cpu cpu,char * objFileName);
void printState(cpu cpu);
void cycleCPU(cpu cpu);
void printStats(cpu cpu);
void reportStage(cpu cpu,enum stage_enum s,const char* fmt,...);

#endif
