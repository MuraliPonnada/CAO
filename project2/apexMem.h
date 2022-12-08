#ifndef APEXMEM_H // Guard against recursive includes
#define APEXMEM_H
#include "apexCPU.h"

int ifetch(cpu cpu);
int dfetch(cpu cpu,int addr);
void dstore(cpu cpu,int addr,int value);

#endif