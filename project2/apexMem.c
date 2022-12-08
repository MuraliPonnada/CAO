#include "apexMem.h"
#include <string.h>
#include <stdio.h>

int ifetch(cpu cpu) {
	int addr=cpu->pc;
	int idx=(addr-0x4000)/4;
	if (idx<0 || idx>=cpu->numInstructions || 0!=addr%4) {
		cpu->stop=1;
		sprintf(cpu->abend,"Segmentation violation in ifetch pc=%08x",addr);
		return 0;
	}
	return cpu->codeMem[idx];
}

int dfetch(cpu cpu,int addr) {
	int idx=addr/4;
	if (idx<0 || idx>127) {
		cpu->stop=1;
		sprintf(cpu->abend,"dfetch segmentation violation - address %08x out of range",addr);
		return 0;
	}
	if (0!=addr%4) {
		cpu->stop=1;
		sprintf(cpu->abend,"dfetch segmentation: %08x not a multiple of 4",addr);
		return 0;
	}
	if (idx<cpu->lowMem) {
		cpu->lowMem=idx;
		printf("Warning... accessing uninitialized memory at address %08x\n",addr);
		cpu->dataMem[idx]=0;
	}
	if (idx>cpu->highMem) {
		cpu->highMem=idx;
		printf("Warning... accessing uninitialized memory at address %08x\n",addr);
		cpu->dataMem[idx]=0;
	}
	return cpu->dataMem[idx];
}

void dstore(cpu cpu,int addr,int value) {
	int idx=addr/4;
	if (idx<0 || idx>127) {
		cpu->stop=1;
		sprintf(cpu->abend,"dfetch segmentation: %08x out of range",addr);
		return;
	}
	if (0!=addr%4) {
		cpu->stop=1;
		sprintf(cpu->abend,"dfetch segmentation: %08x not a multiple of 4",addr);
		return;
	}
	if (idx<cpu->lowMem) cpu->lowMem=idx;
	if (idx>cpu->highMem) cpu->highMem=idx;
	cpu->dataMem[idx]=value;
}