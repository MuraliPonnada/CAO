#ifndef APEXOPCODES_H // Guard against recursive includes
#define APEXOPCODES_H
#include "apexCPU.h"

/*---------------------------------------------------------
  Enumeration of all valid opcode formats
  		See APEX ISA document for complete details
---------------------------------------------------------*/
enum opFormat_enum {
	fmt_nop, // opcode with no operands
	fmt_dss, // opcode, dest, sr1, sr2
	fmt_dsi, // opcode, dest, sr1, imm
	fmt_di, // opcode, dest, x, imm
	fmt_ssi, // opcode, sr2,sr1,imm
	fmt_ss, // opcode, x, sr1, sr2
	fmt_off // opcode, offset
};

/*---------------------------------------------------------
  Enumeration of all valid opcodes
  		See APEX ASI document for complete details
  NOTE: Must be 1-1 with opInfo array below!
---------------------------------------------------------*/
enum opcode_enum {
	NOP,
	ADD,
	ADDL,
	SUB,
	SUBL,
	MUL,
	AND,
	OR,
	XOR,
	MOVC,
	LOAD,
	STORE,
	CMP,
	JUMP,
	BZ,
	BNZ,
	BP,
	BNP,
	HALT
};

#define NUMOPS (HALT+1)

/*---------------------------------------------------------
  Definition of Array of information for each opcode
  		Includes mnemonic string and instruction format
  		See APEX ASI document for complete details

  NOTE: Declaration/initialization of opInfo in apexOpInfo.h
  	must be 1-1 with the opcode_enum above!
---------------------------------------------------------*/
extern struct opInfo_struct {
	char mnemonic[8];
	enum opFormat_enum format;
} opInfo[NUMOPS];

/*---------------------------------------------------------
  Function declarations for externally available functions
---------------------------------------------------------*/
void registerAllOpcodes();
void registerOpcode(int opNum,
	opStageFn decodeFn,opStageFn stg1,
	opStageFn stg2,opStageFn stg3,opStageFn writebackFn);
char * disassemble(int instruction,char *buf);

#endif
