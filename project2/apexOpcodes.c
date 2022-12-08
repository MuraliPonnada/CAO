#include "apexOpcodes.h"
#include "apexCPU.h"
#include "apexMem.h"
#include "apexOpInfo.h" // OpInfo definition
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*---------------------------------------------------------
This file contains a C function for each opcode and
stage that needs to do work for that opcode in that
stage.

When functions for a specific opcode are registered,
the simulator will invoke that function for that operator
when the corresponding stage is cycled. A NULL
function pointer for a specific opcode/stage indicates
that no processing is required for that operation and
stage.

The registerAllOpcodes function should invoke the
registerOpcode function for EACH valid opcode. If
you add new opcodes to be simulated, code the
functions for each stage for that opcode, and add
an invocation of registerOpcode to the
registerAllOpcodes function.
---------------------------------------------------------*/

/*---------------------------------------------------------
  Helper Function Declarations
---------------------------------------------------------*/
void fetch_register1(cpu cpu);
void fetch_register2(cpu cpu);
void check_dest(cpu cpu);
void set_conditionCodes(cpu cpu);
void exForward(cpu cpu);

/*---------------------------------------------------------
  Global Variables
---------------------------------------------------------*/
opStageFn opFns[18][NUMOPS]={NULL}; // Array of function pointers, one for each stage/opcode combination


/*---------------------------------------------------------
  Decode stage functions
---------------------------------------------------------*/
void nop_decode(cpu cpu) {
	cpu->stage[decode].func=alu;
}

void dss_decode(cpu cpu) {
	cpu->stage[decode].status=stage_noAction;
	fetch_register1(cpu);
	fetch_register2(cpu);
	check_dest(cpu);
	if(cpu->stage[decode].opcode==AND || cpu->stage[decode].opcode==OR || cpu->stage[decode].opcode==XOR || cpu->stage[decode].opcode==ADD || cpu->stage[decode].opcode==SUB)
	cpu->stage[decode].func=alu;
	if (cpu->stage[decode].opcode==MUL)
	cpu->stage[decode].func=mul;
}
void dsi_decode(cpu cpu) {
	cpu->stage[decode].status=stage_noAction;
	fetch_register1(cpu);
	check_dest(cpu);
	if(cpu->stage[decode].opcode==ADDL || cpu->stage[decode].opcode==SUBL)
	cpu->stage[decode].func=alu;
	if (cpu->stage[decode].opcode==LOAD)
	cpu->stage[decode].func=ldr;
}

void ssi_decode(cpu cpu) {
	cpu->stage[decode].status=stage_noAction;
	fetch_register1(cpu);
	fetch_register2(cpu);
	if(cpu->stage[decode].opcode==CMP)
	cpu->stage[decode].func=alu;
	if (cpu->stage[decode].opcode==STORE)
	cpu->stage[decode].func=str;
}

void movc_decode(cpu cpu) {
	cpu->stage[decode].status=stage_noAction;
	check_dest(cpu);
	cpu->stage[decode].func=alu;
}

void cbranch_decode(cpu cpu) {
	cpu->stage[decode].branch_taken=0;
	cpu->stage[decode].func=brz;
	if (cpu->stage[decode].opcode==JUMP) cpu->stage[decode].branch_taken=1;
	if (cpu->stage[decode].opcode==BZ && cpu->cc.z) cpu->stage[decode].branch_taken=1;
	if (cpu->stage[decode].opcode==BNZ && !cpu->cc.z) cpu->stage[decode].branch_taken=1;
	if (cpu->stage[decode].opcode==BP && cpu->cc.p) cpu->stage[decode].branch_taken=1;
	if (cpu->stage[decode].opcode==BNP && !cpu->cc.p) cpu->stage[decode].branch_taken=1;
	if (cpu->stage[decode].branch_taken) {
		// Squash instruction currently in fetch
		cpu->stage[fetch].instruction=0;
		cpu->stage[fetch].status=stage_squashed;
		reportStage(cpu,fetch," squashed by previous branch");
		cpu->halt_fetch=1;
		reportStage(cpu,decode," branch taken");
	} else {
	  reportStage(cpu,decode," branch not taken");
  }
}

/*---------------------------------------------------------
  Execute stage functions
---------------------------------------------------------*/
void nop_execute1(cpu cpu) {
	cpu->pipearr[0]=1;
}

void nop_execute2(cpu cpu) {
	cpu->stage[alu1].status = stage_squashed;
	cpu->stage[alu1].instruction = 0;
	cpu->stage[alu1].opcode = 0;
}

void nop_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
	cpu->stage[alu2].status = stage_squashed;
	cpu->stage[alu1].instruction = 0;
	cpu->stage[alu1].opcode = 0;
}

void add_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1+cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"res=%d+%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	set_conditionCodes(cpu);
	exForward(cpu);
}

void add_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void sub_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1-cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"res=%d-%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	set_conditionCodes(cpu);
	exForward(cpu);
}

void sub_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void cmp_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1-cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"cc based on %d-%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	set_conditionCodes(cpu);
	// exForward(cpu);
}

void cmp_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void mul_execute1(cpu cpu) {
	cpu->stage[mul1].result=cpu->stage[mul1].op1*cpu->stage[mul1].op2;
	reportStage(cpu,mul1,"res=%d*%d",cpu->stage[mul1].op1,cpu->stage[mul1].op2);
	set_conditionCodes(cpu);
	exForward(cpu);
}

void mul_execute3(cpu cpu) {
	cpu->pipearr[1]=1;
}

void and_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1&cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"res=%d&%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	exForward(cpu);
}

void and_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void or_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1|cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"res=%d|%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	exForward(cpu);
}

void or_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void xor_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1^cpu->stage[alu1].op2;
	reportStage(cpu,alu1,"res=%d^%d",cpu->stage[alu1].op1,cpu->stage[alu1].op2);
	exForward(cpu);
}

void xor_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void movc_execute1(cpu cpu) {
	cpu->stage[alu1].result=cpu->stage[alu1].op1;
	reportStage(cpu,alu1,"res=%d",cpu->stage[alu1].result);
	exForward(cpu);
}

void movc_execute3(cpu cpu) {
	cpu->pipearr[0]=1;
}

void store_execute1(cpu cpu) {
	cpu->stage[str1].effectiveAddr =
		cpu->stage[str1].op1 + cpu->stage[str1].imm;
	reportStage(cpu,str1,"effAddr=%08x",cpu->stage[str1].effectiveAddr);
}

void store_execute2(cpu cpu) {
	dstore(cpu,cpu->stage[str2].effectiveAddr,cpu->stage[str2].op2);
	reportStage(cpu,str2,"MEM[%06x]=%d",
		cpu->stage[str2].effectiveAddr,
		cpu->stage[str2].op1);
}

void store_execute3(cpu cpu) {
	cpu->pipearr[3]=1;
}

void load_execute1(cpu cpu) {
	cpu->stage[ldr1].effectiveAddr =
		cpu->stage[ldr1].op1 + cpu->stage[ldr1].imm;
	reportStage(cpu,ldr1,"effAddr=%08x",cpu->stage[ldr1].effectiveAddr);
}

void load_execute2(cpu cpu) {
	cpu->stage[ldr2].result = dfetch(cpu,cpu->stage[ldr2].effectiveAddr);
	reportStage(cpu,ldr2,"res=MEM[%06x]",cpu->stage[ldr2].effectiveAddr);
	assert(cpu->mem_fwdBus.valid==0); // load should not have used the ex forwarding bus
	cpu->mem_fwdBus.tag=cpu->stage[ldr2].dr;
	cpu->mem_fwdBus.value=cpu->stage[ldr2].result;
	cpu->mem_fwdBus.valid=1;
}

void load_execute3(cpu cpu) {
	cpu->pipearr[2]=1;
}

void cbranch_execute1(cpu cpu) {
	if (cpu->stage[brz1].branch_taken) {
		// Update PC
		cpu->pc=cpu->stage[brz1].pc+cpu->stage[brz1].offset;
		reportStage(cpu,brz1,"pc=%06x",cpu->pc);
		cpu->halt_fetch=0; // Fetch can start again next cycle
	} else {
		reportStage(cpu,brz1,"No action... branch not taken");
	}
}

void cbranch_execute3(cpu cpu) {
	cpu->pipearr[4]=1;
}

/*---------------------------------------------------------
  Writeback stage functions
---------------------------------------------------------*/
void dest_writeback(cpu cpu) {
	int reg=cpu->stage[writeback].dr;
	cpu->reg[reg]=cpu->stage[writeback].result;
	cpu->regValid[reg]=1;
	reportStage(cpu,writeback,"R%02d<-%d",reg,cpu->stage[writeback].result);
}

void halt_writeback(cpu cpu) {
	cpu->stop=1;
	strcpy(cpu->abend,"HALT instruction retired");
	reportStage(cpu,writeback,"cpu stopped");
}

/*---------------------------------------------------------
  Externally available functions
---------------------------------------------------------*/
void registerAllOpcodes() {
	// Invoke registerOpcode for EACH valid opcode here
	registerOpcode(ADD,dss_decode,add_execute1,NULL,add_execute3,dest_writeback);
	registerOpcode(ADDL,dsi_decode,add_execute1,NULL,add_execute3,dest_writeback);
	registerOpcode(SUB,dss_decode,sub_execute1,NULL,sub_execute3,dest_writeback);
	registerOpcode(SUBL,dsi_decode,sub_execute1,NULL,sub_execute3,dest_writeback);
	registerOpcode(MUL,dss_decode,mul_execute1,NULL,mul_execute3,dest_writeback);
	registerOpcode(AND,dss_decode,and_execute1,NULL,and_execute3,dest_writeback);
	registerOpcode(OR,dss_decode,or_execute1,NULL,or_execute3,dest_writeback);
	registerOpcode(XOR,dss_decode,xor_execute1,NULL,xor_execute3,dest_writeback);
	registerOpcode(MOVC,movc_decode,movc_execute1,NULL,movc_execute3,dest_writeback);
	registerOpcode(LOAD,dsi_decode,load_execute1,load_execute2,load_execute3,dest_writeback);
	registerOpcode(STORE,ssi_decode,store_execute1,store_execute2,store_execute3,NULL);
	registerOpcode(CMP,ssi_decode,cmp_execute1,NULL,cmp_execute3,NULL);
	registerOpcode(JUMP,cbranch_decode,cbranch_execute1,NULL,cbranch_execute3,NULL);
	registerOpcode(BZ,cbranch_decode,cbranch_execute1,NULL,cbranch_execute3,NULL);
	registerOpcode(BNZ,cbranch_decode,cbranch_execute1,NULL,cbranch_execute3,NULL);
	registerOpcode(BP,cbranch_decode,cbranch_execute1,NULL,cbranch_execute3,NULL);
	registerOpcode(BNP,cbranch_decode,cbranch_execute1,NULL,cbranch_execute3,NULL);
	registerOpcode(HALT,nop_decode,nop_execute1,nop_execute2,nop_execute3,halt_writeback);
}

void registerOpcode(int opNum,
	opStageFn decodeFn,opStageFn stg1,
	opStageFn stg2,opStageFn stg3,opStageFn writebackFn) {
	switch(opNum) {
		case MUL:
			opFns[decode][opNum] = decodeFn;
			opFns[mul1][opNum] = stg1;
			opFns[mul2][opNum] = stg2;
			opFns[mul3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case STORE:
			opFns[decode][opNum] = decodeFn;
			opFns[str1][opNum] = stg1;
			opFns[str2][opNum] = stg2;
			opFns[str3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case LOAD:
			opFns[decode][opNum] = decodeFn;
			opFns[ldr1][opNum] = stg1;
			opFns[ldr2][opNum] = stg2;
			opFns[ldr3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case BZ:
			opFns[decode][opNum] = decodeFn;
			opFns[brz1][opNum] = stg1;
			opFns[brz2][opNum] = stg2;
			opFns[brz3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case BNZ:
			opFns[decode][opNum] = decodeFn;
			opFns[brz1][opNum] = stg1;
			opFns[brz2][opNum] = stg2;
			opFns[brz3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case BP:
			opFns[decode][opNum] = decodeFn;
			opFns[brz1][opNum] = stg1;
			opFns[brz2][opNum] = stg2;
			opFns[brz3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case BNP:
			opFns[decode][opNum] = decodeFn;
			opFns[brz1][opNum] = stg1;
			opFns[brz2][opNum] = stg2;
			opFns[brz3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		case JUMP:
			opFns[decode][opNum] = decodeFn;
			opFns[brz1][opNum] = stg1;
			opFns[brz2][opNum] = stg2;
			opFns[brz3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
			break;
		default:
			opFns[decode][opNum] = decodeFn;
			opFns[alu1][opNum] = stg1;
			opFns[alu2][opNum] = stg2;
			opFns[alu3][opNum] = stg3;
			opFns[writeback][opNum] = writebackFn;
	}
}

char * disassemble(int instruction,char *buf) {
	// assumes buf is big enough to hold the full disassemble string (max is probably 32)
	int opcode=(instruction>>24);
	if (opcode>HALT || opcode<0) {
		printf("In disassemble, invalid opcode: %d\n",opcode);
		strcpy(buf,"????");
		return buf;
	}
	buf[0]='\0';
	int dr,sr1,sr2,offset;
	short imm;
	enum opFormat_enum fmt=opInfo[opcode].format;
	switch(fmt) {
		case fmt_nop:
			sprintf(buf,"%s",opInfo[opcode].mnemonic);
			break;
		case fmt_dss:
			dr=(instruction&0x00f00000)>>20;
			sr1=(instruction&0x000f0000)>>16;
			sr2=(instruction&0x0000f000)>>12;
			sprintf(buf,"%s R%02d,R%02d,R%02d",opInfo[opcode].mnemonic,dr,sr1,sr2);
			break;
		case fmt_dsi:
			dr=(instruction&0x00f00000)>>20;
			sr1=(instruction&0x000f0000)>>16;
			imm=(instruction&0x0000ffff);
			sprintf(buf,"%s R%02d,R%02d,#%d",opInfo[opcode].mnemonic,dr,sr1,imm);
			break;
		case fmt_di:
			dr=(instruction&0x00f00000)>>20;
			imm=(instruction&0x0000ffff);
			sprintf(buf,"%s R%02d,#%d",opInfo[opcode].mnemonic,dr,imm);
			break;
		case fmt_ssi:
			sr2=(instruction&0x00f00000)>>20;
			sr1=(instruction&0x000f0000)>>16;
			imm=(instruction&0x0000ffff);
			sprintf(buf,"%s R%02d,R%02d,#%d",opInfo[opcode].mnemonic,sr2,sr1,imm);
			break;
		case fmt_ss:
			sr1=(instruction&0x000f0000)>>16;
			sr2=(instruction&0x0000f000)>>12;
			sprintf(buf,"%s R%02d,R%02d",opInfo[opcode].mnemonic,sr1,sr2);
			break;
		case fmt_off:
			offset=((instruction&0x00ffffff)<<8)>>8; // shift left 8, then right 8 to propagate sign bit
			sprintf(buf,"%s #%d",opInfo[opcode].mnemonic,offset);
			break;
		default :
			printf("In disassemble, format not recognized: %d\n",fmt);
			strcpy(buf,"????");
	}
	return buf;
}

/*---------------------------------------------------------
  Internal helper functions
---------------------------------------------------------*/
void fetch_register1(cpu cpu) {
	int reg=cpu->stage[decode].sr1;
	// Check forwarding busses in program order
	if (cpu->ex_fwdBus.valid && reg==cpu->ex_fwdBus.tag) {
		cpu->stage[decode].op1=cpu->ex_fwdBus.value;
		reportStage(cpu,decode," R%d=%d fwd from EX",reg,cpu->ex_fwdBus.value);
		return;
	}
	if (cpu->mem_fwdBus.valid && reg==cpu->mem_fwdBus.tag) {
		cpu->stage[decode].op1=cpu->mem_fwdBus.value;
		reportStage(cpu,decode," R%d=%d fwd from MEM",reg,cpu->mem_fwdBus.value);
		return;
	}
	if (cpu->regValid[reg]) {
		cpu->stage[decode].op1=cpu->reg[reg];
		reportStage(cpu,decode," R%d=%d",reg,cpu->reg[reg]);
		return;
	}
	// Register value cannot be found
	cpu->stage[decode].status=stage_stalled;
	reportStage(cpu,decode," R%d invalid",reg);
	return;
}

void fetch_register2(cpu cpu) {
	int reg=cpu->stage[decode].sr2;
	// Check forwarding busses in program order
	if (cpu->ex_fwdBus.valid && reg==cpu->ex_fwdBus.tag) {
		cpu->stage[decode].op2=cpu->ex_fwdBus.value;
		reportStage(cpu,decode," R%d=%d fwd from EX",reg,cpu->ex_fwdBus.value);
		return;
	}
	if (cpu->mem_fwdBus.valid && reg==cpu->mem_fwdBus.tag) {
		cpu->stage[decode].op2=cpu->mem_fwdBus.value;
		reportStage(cpu,decode," R%d=%d fwd from MEM",reg,cpu->mem_fwdBus.value);
		return;
	}
	if (cpu->regValid[reg]) {
		cpu->stage[decode].op2=cpu->reg[reg];
		reportStage(cpu,decode," R%d=%d",reg,cpu->reg[reg]);
		return;
	}
	// reg2 value cannot be found
	cpu->stage[decode].status=stage_stalled;
	reportStage(cpu,decode," R%d invalid",reg);
}

void check_dest(cpu cpu) {
	int reg=cpu->stage[decode].dr;
	if (!cpu->regValid[reg]) {
		cpu->stage[decode].status=stage_stalled;
		reportStage(cpu,decode," R%d invalid",reg);
	}
	if (cpu->stage[decode].status!=stage_stalled)  {
		 cpu->regValid[cpu->stage[decode].dr]=0;
		 reportStage(cpu,decode," invalidate R%d",reg);
	}
}

void set_conditionCodes(cpu cpu) {
	// Condition codes always set during the execute phase
	if (cpu->stage[alu1].result==0) cpu->cc.z=1;
	else cpu->cc.z=0;
	if (cpu->stage[alu1].result>0) cpu->cc.p=1;
	else cpu->cc.p=0;
}

void exForward(cpu cpu) {
	cpu->ex_fwdBus.tag=cpu->stage[alu1].dr;
	cpu->ex_fwdBus.value=cpu->stage[alu1].result;
	cpu->ex_fwdBus.valid=1;
}
