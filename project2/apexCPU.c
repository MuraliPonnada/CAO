#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "apexCPU.h"
#include "apexMem.h"

/*---------------------------------------------------------
   Internal function declarations
---------------------------------------------------------*/
void cycle_fetch(cpu cpu);
void cycle_decode(cpu cpu);
void cycle_stage(cpu cpu,int stage);
char * getInum(cpu cpu,int pc);
void reportReg(cpu cpu,int r);

/*---------------------------------------------------------
   Global Variables
---------------------------------------------------------*/
char *stageName[18]={"fetch","decode","alu1","alu2","alu3","mul1","mul2","mul3","ldr1","ldr2","ldr3","str1","str2","str3","brz1","brz2","brz3","writeback"};
extern opStageFn opFns[6][NUMOPS]; // Array of function pointers, one for each stage/opcode combination

/*---------------------------------------------------------
   External Function definitions
---------------------------------------------------------*/

void initCPU(cpu cpu) {
	cpu->pc=0x4000;
	cpu->numInstructions=0;
	cpu->lowMem=128;
	cpu->highMem=-1;
	for(int i=0;i<16;i++) {
		cpu->reg[i]=0xdeadbeef;
		cpu->regValid[i]=1; // all registers start out as "valid"
	}
	cpu->cc.z=cpu->cc.p=0;
	cpu->t=0;
	cpu->instr_retired=0;
	cpu->halt_fetch=0;
	cpu->stop=0;
	for(int i=0;i<18;i++) {
		cpu->stage[i].status=stage_squashed;
		cpu->stage[i].report[0]='\0';
		reportStage(cpu,i,"---");
		cpu->stage[i].instruction=0;
		cpu->stage[i].opcode=0;
		cpu->stage[i].pc=-1;
		cpu->stage[i].branch_taken=0;
		for(int op=0;op<NUMOPS;op++) opFns[i][op]=NULL;
	}
	cpu->ex_fwdBus.valid=0;
	cpu->mem_fwdBus.valid=0;
	for(int i=0;i<5;i++) {
		cpu->pipearr[i]=0;
	}
	registerAllOpcodes();
}

void loadCPU(cpu cpu,char * objFileName) {
	char cmtBuf[128];
	FILE * objF=fopen(objFileName,"r");
	if (objF==NULL) {
		perror("Error - unable to open object file for read");
		printf("...Trying to read from object file %s\n",objFileName);
		return;
	}

	int nread=0;
	while(!feof(objF)) {
		int newInst;
		if (1==fscanf(objF," %08x",&newInst)) {
			cpu->codeMem[nread++]=newInst;
		} else if (1==fscanf(objF,"; %127[^\n]\n",cmtBuf)) {
			// Ignore comments on the same line
			// printf("Ignoring commment: %s\n",cmtBuf);
		} else {
			fscanf(objF," %s ",cmtBuf);
			printf("Load aborted, unrecognized object code: %s\n",cmtBuf);
			return;
		}
	}
	cpu->numInstructions=nread;
	cpu->pc=0x4000;
	cpu->halt_fetch=cpu->stop=0;
	printf("Loaded %d instructions starting at adress 0x4000\n",nread);
}

void printState(cpu cpu) {

	printf("\nCPU state at cycle %d, pc=0x%08x, cc.z=%s cc.p=%s\n",
		cpu->t,cpu->pc,cpu->cc.z?"true":"false",cpu->cc.p?"true":"false");

	printf("Stage Info:\n");
	char instBuf[32];
	for (int s=0;s<18;s++) {
		printf("  %10s: pc=%05x %s",stageName[s],cpu->stage[s].pc,disassemble(cpu->stage[s].instruction,instBuf));
		if (cpu->stage[s].status==stage_squashed) printf(" squashed");
		if (cpu->stage[s].status==stage_stalled) printf(" stalled");
		printf(" %s\n",cpu->stage[s].report);
	}

   printf("\n Int Regs: \n   ");
   for(int r=0;r<16;r++) reportReg(cpu,r);
	printf("\n");

	if ((cpu->lowMem<128) && (cpu->highMem>-1)) {
		printf("Modified memory:\n");
		for(int i=cpu->lowMem;i<=cpu->highMem;i++) {
			printf("MEM[%04x]=%d\n",i*4,cpu->dataMem[i]);
		}
		printf("\n");
	}

	if (cpu->ex_fwdBus.valid) {
		printf("Forward bus from EX: R%d, value=%d\n",
			cpu->ex_fwdBus.tag,cpu->ex_fwdBus.value);
	}
	if (cpu->mem_fwdBus.valid) {
		printf("Forward bus from MEM: R%d, value=%d\n",
			cpu->mem_fwdBus.tag,cpu->mem_fwdBus.value);
	}

	if (cpu->halt_fetch) {
		printf("Instruction fetch is halted.\n");
	}
	if (cpu->stop) {
		printf("CPU is stopped because %s\n",cpu->abend);
	}
}

void cycleCPU(cpu cpu) {
	if (cpu->stop) {
		printf("CPU is stopped for %s. No cycles allowed.\n",cpu->abend);
		return;
	}

	// Move register information down one stage
	//    backwards so that you don't overwrite
	if (cpu->stage[writeback].status==stage_stalled) {
		cpu->stop=1;
		strcpy(cpu->abend,"Writeback stalled - no progress possible");
	} 
	else {
		int c=0;
		for(int i=0;i<5;i++)
		{
			if(cpu->pipearr[i]!=1)
			c++;
		}
		
		if (c==5)
		{
			cpu->stage[writeback].status = stage_squashed;
			cpu->stage[writeback].instruction = 0;
			cpu->stage[writeback].opcode = 0;
		}
		
		if(cpu->pipearr[0]==0)
		{
			cpu->stage[alu3] = cpu->stage[alu2];
			cpu->stage[alu2] = cpu->stage[alu1];
		}
		
		if(cpu->pipearr[0]==1)
		{
			cpu->stage[writeback] = cpu->stage[alu3];
			cpu->stage[alu3] = cpu->stage[alu2];
			cpu->stage[alu2] = cpu->stage[alu1];
			cpu->pipearr[0]=0;
		}
		
		if(cpu->pipearr[1]==0)
		{
			cpu->stage[mul3] = cpu->stage[mul2];
			cpu->stage[mul2] = cpu->stage[mul1];
		}
		
		if(cpu->pipearr[1]==1)
		{
			cpu->stage[writeback] = cpu->stage[mul3];
			cpu->stage[mul3] = cpu->stage[mul2];
			cpu->stage[mul2] = cpu->stage[mul1];
			cpu->pipearr[1]=0;
		}
		
		if(cpu->pipearr[2]==0)
		{
			cpu->stage[ldr3] = cpu->stage[ldr2];
			cpu->stage[ldr2] = cpu->stage[ldr1];
		}
		
		if(cpu->pipearr[2]==1)
		{
			cpu->stage[writeback] = cpu->stage[ldr3];
			cpu->stage[ldr3] = cpu->stage[ldr2];
			cpu->stage[ldr2] = cpu->stage[ldr1];
			cpu->pipearr[2]=0;
		}
		
		if(cpu->pipearr[3]==0)
		{
			cpu->stage[str3] = cpu->stage[str2];
			cpu->stage[str2] = cpu->stage[str1];
		}
		
		if(cpu->pipearr[3]==1)
		{
			cpu->stage[writeback] = cpu->stage[str3];
			cpu->stage[str3] = cpu->stage[str2];
			cpu->stage[str2] = cpu->stage[str1];
			cpu->pipearr[3]=0;
		}
		
		if(cpu->pipearr[4]==0)
		{
			cpu->stage[brz3] = cpu->stage[brz2];
			cpu->stage[brz2] = cpu->stage[brz1];
		}
		
		if(cpu->pipearr[4]==1)
		{
			cpu->stage[writeback] = cpu->stage[brz3];
			cpu->stage[brz3] = cpu->stage[brz2];
			cpu->stage[brz2] = cpu->stage[brz1];
			cpu->pipearr[4]=0;
		}
		
		if(cpu->stage[decode].func == alu && cpu->stage[decode].status != stage_stalled)
		{
				cpu->stage[alu1] = cpu->stage[decode];
		}
		else 
		{
				cpu->stage[alu1].status = stage_squashed;
				cpu->stage[alu1].instruction = 0;
				cpu->stage[alu1].opcode = 0;
		}
		
		if(cpu->stage[decode].func == mul && cpu->stage[decode].status != stage_stalled)
		{
				cpu->stage[mul1] = cpu->stage[decode];
		}
		else
		{
				cpu->stage[mul1].status = stage_squashed;
				cpu->stage[mul1].instruction = 0;
				cpu->stage[mul1].opcode = 0;
		}
		
		if(cpu->stage[decode].func == str && cpu->stage[decode].status != stage_stalled)
		{
				cpu->stage[str1] = cpu->stage[decode];
				
		}
		else
		{
				cpu->stage[str1].status = stage_squashed;
				cpu->stage[str1].instruction = 0;
				cpu->stage[str1].opcode = 0;
		}
		
		if(cpu->stage[decode].func == ldr && cpu->stage[decode].status != stage_stalled)
		{
				cpu->stage[ldr1] = cpu->stage[decode];
		}
		else
		{
				cpu->stage[ldr1].status = stage_squashed;
				cpu->stage[ldr1].instruction = 0;
				cpu->stage[ldr1].opcode = 0;
		}
		
		if(cpu->stage[decode].func == brz && cpu->stage[decode].status != stage_stalled)
		{
				cpu->stage[brz1] = cpu->stage[decode];
		}
		else
		{
				cpu->stage[brz1].status = stage_squashed;
				cpu->stage[brz1].instruction = 0;
				cpu->stage[brz1].opcode = 0;
		}
		
		if(cpu->stage[decode].status != stage_stalled)
		cpu->stage[decode] = cpu->stage[fetch];
		
	}

	// Move data from the EX fwd bus to the MEM fwd bus if valid
	if (cpu->ex_fwdBus.valid) {
		cpu->mem_fwdBus=cpu->ex_fwdBus; // Copy all fields
	} else cpu->mem_fwdBus.valid=0;
	cpu->ex_fwdBus.valid=0;

	// Reset the reports and status as required for all stages
	for(int s=0;s<18;s++) {
		cpu->stage[s].report[0]='\0';
		switch (cpu->stage[s].status) {
			case stage_squashed:
			case stage_stalled:
			case stage_noAction:
				break; // No change required
			case stage_actionComplete:
				cpu->stage[s].status=stage_noAction; // Overwrite previous stages status
		}
	}

	// Cycle all eighteen stages
	if (!cpu->stop) cycle_fetch(cpu);
	if (!cpu->stop) cycle_decode(cpu); // Do the decode part of d/rf
	if (!cpu->stop) cycle_stage(cpu,alu1);
	if (!cpu->stop) cycle_stage(cpu,alu2);
	if (!cpu->stop) cycle_stage(cpu,alu3);
	if (!cpu->stop) cycle_stage(cpu,mul1);
	if (!cpu->stop) cycle_stage(cpu,mul2);
	if (!cpu->stop) cycle_stage(cpu,mul3);
	if (!cpu->stop) cycle_stage(cpu,ldr1);
	if (!cpu->stop) cycle_stage(cpu,ldr2);
	if (!cpu->stop) cycle_stage(cpu,ldr3);
	if (!cpu->stop) cycle_stage(cpu,str1);
	if (!cpu->stop) cycle_stage(cpu,str2);
	if (!cpu->stop) cycle_stage(cpu,str3);
	if (!cpu->stop) cycle_stage(cpu,brz1);
	if (!cpu->stop) cycle_stage(cpu,brz2);
	if (!cpu->stop) cycle_stage(cpu,brz3);
	if (!cpu->stop) cycle_stage(cpu,writeback);

	if (!cpu->stop) cycle_stage(cpu,decode); // Do the rf part of d/rf

	cpu->t++; // update the clock tick - This cycle has completed
	if (cpu->t==1) {
		printf("      |ftch|deco|alu1|alu2|alu3|mul1|mul2|mul3|lod1|lod2|lod3|sto1|sto2|sto3|br1 |br2 |br3 | wb |\n");
	}

	// Report on all eighteen stages (move this before cycling the rf part of decode to match Kanad's results)
	printf ("t=%3d |",cpu->t);
	for(int s=0;s<18;s++) {
		int stalled=0;
		for(int f=s;f<18;f++) if (cpu->stage[f].status==stage_stalled) stalled=1;
		if (stalled) printf ("%3ss|", getInum(cpu,cpu->stage[s].pc));
		else {
			switch(cpu->stage[s].status) {
				case stage_squashed: printf("   q|"); break;
				case stage_stalled: break; // printed stalled above
				case stage_noAction: printf ("%3s-|", getInum(cpu,cpu->stage[s].pc)); break;
				case stage_actionComplete: printf("%3s+|", getInum(cpu,cpu->stage[s].pc)); break;
			}
		}

	}
	printf("\n");

	// if (!cpu->stop) cycle_stage(cpu,decode); // Do the rf part of d/rf

	if (cpu->stop) {
		printf("CPU stopped because %s\n",cpu->abend);
	}
}

void printStats(cpu cpu) {
	printf("\nAPEX Simulation complete.\n");
	printf("    Total cycles executed: %d\n",cpu->t);
	printf("    Instructions retired: %d\n",cpu->instr_retired);
	printf("    Instructions per Cycle (IPC): %5.3f\n",((float)cpu->instr_retired)/cpu->t);
	printf("    Stop is %s\n",cpu->stop?"true":"false");
	if (cpu->stop) {
		printf("    Reason for stop: %s\n",cpu->abend);
	}
}

void reportStage(cpu cpu,enum stage_enum s,const char* fmt,...) {
	char msgBuf[1024]={0};
	va_list args;
	va_start(args,fmt);
	vsprintf(msgBuf,fmt,args);
	va_end(args);
	// Truncate msgBuf to fit
	if (strlen(msgBuf)+strlen(cpu->stage[s].report) >=128) {
		msgBuf[127-strlen(cpu->stage[s].report)]='\0';
	}
	strcat(cpu->stage[s].report,msgBuf);
}

/*---------------------------------------------------------
   Internal Function definitions
---------------------------------------------------------*/

void cycle_fetch(cpu cpu) {
	// Don't run if anything downstream is stalled
	for(int s=1;s<18;s++) if (cpu->stage[s].status==stage_stalled) return;
	if (cpu->halt_fetch) {
		cpu->stage[fetch].status=stage_squashed;
		cpu->stage[fetch].instruction=0;
		cpu->stage[fetch].opcode=0;
		return;
	}
	int inst=ifetch(cpu);
	if (!cpu->stop) {
		cpu->stage[fetch].status=stage_noAction;
		cpu->stage[fetch].instruction=inst;
		cpu->stage[fetch].opcode=(inst>>24);
		if (cpu->stage[fetch].opcode<0 || cpu->stage[fetch].opcode>HALT) {
			cpu->stop=1;
			sprintf(cpu->abend,"Invalid opcode %x after ifetch(%08x)",
				cpu->stage[fetch].opcode,cpu->pc);
			return;
		}
		reportStage(cpu,fetch,"ifetch %s",getInum(cpu,cpu->pc));
		if (cpu->stage[fetch].opcode==HALT) {
			cpu->halt_fetch=1; // Stop fetching when the HALT instruction is fetched
			reportStage(cpu,fetch," --- fetch halted");
		}
		cpu->stage[fetch].pc=cpu->pc;
		cpu->stage[fetch].status=stage_actionComplete;
		cpu->pc+=4;
	}
}

void cycle_decode(cpu cpu) {
	// Does the first half (the decode part) of the decode/fetch regs stage
	if (cpu->stage[decode].status==stage_squashed) return;
	if (cpu->stage[decode].status==stage_stalled) return; // Decode already done
	enum opFormat_enum fmt=opInfo[cpu->stage[decode].opcode].format;
	int inst=cpu->stage[decode].instruction;
	switch(fmt) {
		case fmt_nop:
			reportStage(cpu,decode,"decode(nop)");
			break; // No decoding required
		case fmt_dss:
			cpu->stage[decode].dr=(inst&0x00f00000)>>20;
			cpu->stage[decode].sr1=(inst&0x000f0000)>>16;
			cpu->stage[decode].sr2=(inst&0x0000f000)>>12;
			reportStage(cpu,decode,"decode(dss)");
			break;
		case fmt_dsi:
			cpu->stage[decode].dr=(inst&0x00f00000)>>20;
			cpu->stage[decode].sr1=(inst&0x000f0000)>>16;
			cpu->stage[decode].imm=((inst&0x0000ffff)<<16)>>16;
			cpu->stage[decode].op2=cpu->stage[decode].imm;
			reportStage(cpu,decode,"decode(dsi) op2=%d",cpu->stage[decode].op2);
			break;
		case fmt_di:
			cpu->stage[decode].dr=(inst&0x00f00000)>>20;
			cpu->stage[decode].imm=((inst&0x0000ffff)<<16)>>16; // Shift left/right to propagate sign bit
			cpu->stage[decode].op1=cpu->stage[decode].imm;
			reportStage(cpu,decode,"decode(di) op1=%d",cpu->stage[decode].op1);
			break;
		case fmt_ssi:
			cpu->stage[decode].sr2=(inst&0x00f00000)>>20;
			cpu->stage[decode].sr1=(inst&0x000f0000)>>16;
			cpu->stage[decode].imm=((inst&0x0000ffff)<<16)>>16;
			reportStage(cpu,decode,"decode(ssi) imm=%d",cpu->stage[decode].imm);
			break;
		case fmt_ss:
			cpu->stage[decode].sr1=(inst&0x000f0000)>>16;
			cpu->stage[decode].sr2=(inst&0x0000f000)>>12;
			reportStage(cpu,decode,"decode(ss)");
			break;
		case fmt_off:
			cpu->stage[decode].offset=((inst&0x0000ffff)<<16)>>16;
			reportStage(cpu,decode,"decode(off)");
			break;
		default :
			cpu->stop=1;
			sprintf(cpu->abend,"Decode format %d not recognized in cycle_decode",fmt);
	}
}

void cycle_stage(cpu cpu,int stage) {
	for(int s=stage+1;s<18;s++) if (cpu->stage[s].status==stage_stalled) return;
	if (cpu->stage[stage].status==stage_squashed) return;
	assert(stage>=0 && stage<=writeback);
	assert(cpu->stage[stage].opcode>=0 && cpu->stage[stage].opcode<=HALT);
	opStageFn stageFn=opFns[stage][cpu->stage[stage].opcode];
	if (stageFn) {
		stageFn(cpu);
		if (cpu->stage[stage].status==stage_noAction)
			cpu->stage[stage].status=stage_actionComplete;
	} else {
		cpu->stage[stage].status=stage_noAction;
	}
	if (stage==writeback && cpu->stage[writeback].status!=stage_squashed) {
		cpu->instr_retired++;
	}
}

char * getInum(cpu cpu,int pc) {
	static char inumBuf[5];
	inumBuf[0]=0x00;
	if (pc==-1) return inumBuf;
	int n=(pc-0x4000)/4;
	if (n<0 || n>cpu->numInstructions) {
		cpu->stop=1;
		sprintf(cpu->abend,"in getInum, pc was %x\n",pc);
		n=-1;
	}
	sprintf(inumBuf,"I%d",n);
	return inumBuf;
}

void reportReg(cpu cpu,int r) {
	int v=cpu->reg[r];
	printf("R%02d",r);
	if (cpu->regValid[r]) {
		if (v!=0xdeadbeef) printf("=%05d ",v);
	   else printf(" ----- ");
	} else printf(" xxxxx ");
	if (7==r%8) printf("\n   ");
}
