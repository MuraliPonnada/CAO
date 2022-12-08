#ifndef APEXOPINFO_H
#define APEXOPINFO_H
#include "apexOpcodes.h" // To get NUMOPS

/*---------------------------------------------------------
  Array of information for each opcode
  		Includes mnemonic string and instruction format
  		See APEX ISA document for complete details

  NOTE: Declaration/initialization in apexOpcodes.c
  	must be 1-1 with the opcode_enum in apexOpcodes.h!
---------------------------------------------------------*/
struct opInfo_struct opInfo[NUMOPS]  ={
	{"NOP",fmt_nop},
	{"ADD",fmt_dss},
	{"ADDL",fmt_dsi},
	{"SUB",fmt_dss},
	{"SUBL",fmt_dsi},
	{"MUL",fmt_dss},
	{"AND",fmt_dss},
	{"OR",fmt_dss},
	{"XOR",fmt_dss},
	{"MOVC",fmt_di},
	{"LOAD",fmt_dsi},
	{"STORE",fmt_ssi},
	{"CMP",fmt_ss},
	{"JUMP",fmt_off},
	{"BZ",fmt_off},
	{"BNZ",fmt_off},
	{"BP",fmt_off},
	{"BNP",fmt_off},
	{"HALT",fmt_nop}
};

#endif