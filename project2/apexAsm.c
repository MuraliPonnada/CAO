#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "apexOpcodes.h" // Use the same set of opcodes as the simulator!
#include "apexOpInfo.h" // Include definition of APEX opcodes

extern char* strdup(const char*); // Prototype not in string.h when c standard>c99

/*---------------------------------------------------------
  Function declarations for internal functions
---------------------------------------------------------*/
int tokenize(int lnum,char *line,char * tokens[],int tokenPos[]);
void printEmsg(char *line,int lnum,int col,char *txt);
void strUpper(char * string);
int getRegister(char *string);
int getImmediate(char *string);
int makeInstruction(unsigned char opNum,enum opFormat_enum  format,int dr,int sr1,int sr2,int imm,int offset);

/*---------------------------------------------------------
  Global Variables
---------------------------------------------------------*/
#define LINELENGTH 128
#define MAXTOKENS 10

/*---------------------------------------------------------
  Main function
  		command line args: assembly file name

  		Reads the assembly file name and creates an
  		object file (replacing .s with .o) that contains
  		the APEX "binary" instructions read from the
  		assembly file.
---------------------------------------------------------*/
int main(int argc,char **argv) {
	char * asmFile;
	if (argc<2) {
		printf("Invoke as %s <asmFile.s>\n",argv[0]);
		return 1;
	}

	asmFile=argv[1];
	char * objFile=strdup(asmFile);
	int dp=strlen(objFile)-2;
	if (strcmp(objFile+dp,".s")!=0) {
		printf("Error - asmFile name: %s must end in .s\n",asmFile);
		return 1;
	}
	objFile[dp+1]='o';
	FILE * asmF=fopen(asmFile,"r");
	if (asmF==NULL) {
		perror("Error - unable to open assembler file for read\n");
		free(objFile);
		return 1;
	}

	FILE * objF=fopen(objFile,"w");
	if (objF==NULL) {
		perror("Error - unable to open object output file for write\n");
		fclose(asmF);
		free(objFile);
		return 1;
	}

	printf("Info - Assembling from %s into %s\n",asmFile,objFile);

	char asmLine[LINELENGTH];
	char * token[MAXTOKENS];
	int tokenPos[MAXTOKENS];

	int lineNum=0;
	int inum=0; // Instruction number
	while(!feof(asmF)) {
		if (NULL==fgets(asmLine,LINELENGTH,asmF)) {
			if (feof(asmF)) continue;
			perror("Error - reading from asmFile:");
			fclose(asmF);
			fclose(objF);
			free(objFile);
			return 1;
		}
		lineNum++;
		// Strip trailing newline if there
		int ll=strlen(asmLine);
		if (asmLine[ll-1]=='\n') asmLine[ll-1]='\0';
		int ntokens=tokenize(lineNum,asmLine,token,tokenPos);
		if (ntokens==0) continue; // Empty line
		// Convert first token to uppercase
		strUpper(token[0]);
		// Find opcode that matches token[0] mnemonic
		int opcode=-1;
		for(int i=0;i<NUMOPS;i++) {
			if (0==strcmp(token[0],opInfo[i].mnemonic)) {
				opcode=i;
				break;
			}
		}
		if (opcode==-1) {
			printEmsg(asmLine,lineNum,tokenPos[0],"Invalid opcode mnemonic");
			continue;
		}
		int dr,sr1,sr2,imm,offset;
		// Remaining token requirements deped on opcode format
		enum opFormat_enum format =opInfo[opcode].format;
		switch(format) {
			case fmt_nop:
				if (ntokens>1) {
					printEmsg(asmLine,lineNum,tokenPos[1],"Extra token(s) ignored");
				}
				break;
			case fmt_dss:
				dr=sr1=sr2=-1;
				if (ntokens>1) {
					dr=getRegister(token[1]);
					if (dr<0 || dr>15) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid destination register");
						dr=-1;
					}
					if (ntokens>2) {
						sr1=getRegister(token[2]);
						if (sr1<0 || sr1>15) {
							printEmsg(asmLine,lineNum,tokenPos[2],"Invalid first source register");
							sr1=-1;
						}
						if (ntokens>3) {
							sr2=getRegister(token[3]);
							if (sr2<0 || sr2>15) {
								printEmsg(asmLine,lineNum,tokenPos[3],"Invalid second source register");
								sr2=-1;
							}
							if (ntokens>4) printEmsg(asmLine,lineNum,tokenPos[3],"Extra tokens ignored");
						} else printEmsg(asmLine,lineNum,tokenPos[2],"Missing second source register");
					} else printEmsg(asmLine,lineNum,tokenPos[1],"Missing first source register");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing destination register");
				break;

			case fmt_dsi:
				dr=sr1=-1; imm=INT_MIN;
				if (ntokens>1) {
					dr=getRegister(token[1]);
					if (dr<0 || dr>15) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid destination register");
						dr=-1;
					}
					if (ntokens>2) {
						sr1=getRegister(token[2]);
						if (sr1<0 || sr1>15) {
							printEmsg(asmLine,lineNum,tokenPos[2],"Invalid first source register");
							sr1=-1;
						}
						if (ntokens>3) {
							imm=getImmediate(token[3]);
							if (imm==INT_MIN) {
								printEmsg(asmLine,lineNum,tokenPos[3],"Invalid immediate value");
								sr2=-1;
							}
							if (ntokens>4) printEmsg(asmLine,lineNum,tokenPos[3],"Extra tokens ignored");
						} else printEmsg(asmLine,lineNum,tokenPos[2],"Missing immediate value");
					} else printEmsg(asmLine,lineNum,tokenPos[1],"Missing first source register");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing destination register");
				break;

			case fmt_di:
				dr=-1; imm=INT_MIN;
				if (ntokens>1) {
					dr=getRegister(token[1]);
					if (dr<0 || dr>15) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid destination register");
						dr=-1;
					}
					if (ntokens>2) {
						imm=getImmediate(token[2]);
						if (imm==INT_MIN) {
							printEmsg(asmLine,lineNum,tokenPos[2],"Invalid immediate value");
						}
						if (ntokens>3) printEmsg(asmLine,lineNum,tokenPos[2],"Extra tokens ignored");
					} else printEmsg(asmLine,lineNum,tokenPos[1],"Missing immediate value");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing destination register");
				break;

			case fmt_ssi:
				sr2=sr1=-1; imm=INT_MIN;
				if (ntokens>1) {
					sr2=getRegister(token[1]);
					if (sr2<0 || sr2>15) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid second source register");
						sr2=-1;
					}
					if (ntokens>2) {
						sr1=getRegister(token[2]);
						if (sr1<0 || sr1>15) {
							printEmsg(asmLine,lineNum,tokenPos[2],"Invalid first source register");
							sr1=-1;
						}
						if (ntokens>3) {
							imm=getImmediate(token[3]);
							if (imm==INT_MIN) {
								printEmsg(asmLine,lineNum,tokenPos[3],"Invalid immediate value");
								sr2=-1;
							}
							if (ntokens>4) printEmsg(asmLine,lineNum,tokenPos[4],"Extra tokens ignored");
						} else printEmsg(asmLine,lineNum,tokenPos[2],"Missing immediate value");
					} else printEmsg(asmLine,lineNum,tokenPos[1],"Missing first source register");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing destination register");
				break;

			case fmt_ss:
				sr1=sr2=-1;
				if (ntokens>1) {
					sr1=getRegister(token[1]);
					if (sr1<0 || sr1>15) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid first source register");
						sr1=-1;
					}
					if (ntokens>2) {
						sr2=getRegister(token[2]);
						if (sr2<0 || sr2>15) {
							printEmsg(asmLine,lineNum,tokenPos[2],"Invalid second source register");
							sr2=-1;
						}
						if (ntokens>3) printEmsg(asmLine,lineNum,tokenPos[3],"Extra tokens ignored");
					} else printEmsg(asmLine,lineNum,tokenPos[1],"Missing second source register");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing first source register");
				break;

			case fmt_off:
				offset=INT_MIN;
				if (ntokens>1) {
					offset=getImmediate(token[1]);
					if (offset==INT_MIN) {
						printEmsg(asmLine,lineNum,tokenPos[1],"Invalid offset");
					}
					if (ntokens>2) printEmsg(asmLine,lineNum,tokenPos[2],"Extra tokens ignored");
				} else printEmsg(asmLine,lineNum,tokenPos[0],"Missing offset");
				break;
		}
		free(token[0]);

		if (opcode!=-1) {
			int inst=makeInstruction(opcode,format,dr,sr1,sr2,imm,offset);
			fprintf(objF,"%08x ; %3d : %s\n",inst,inum,asmLine);
			printf(" %3d=I%d | %08x | %s\n",lineNum,inum,inst,asmLine);
			inum++;
		} else {
			// printf(" %3d |          | %s\n",lineNum,asmLine);
		}
	} // End of loop through assembly file

	fclose(asmF);
	fclose(objF);
	free(objFile);
	return 0;
}

/*---------------------------------------------------------
  tokenize: reads a line and updates an array of tokens
  		A token is a white-space or comma delimited string
  		"tokens" is an array of pointers to each token in the line.
  		The line is updated to put end-of-string (0x00) after each token
  		"tokenPos" keeps track of the location of each token in the input line
  		returns the number of tokens found.
---------------------------------------------------------*/
int tokenize(int lnum,char *line,char * tokens[],int tokenPos[]) {
	int ntokes=0; // Note... comment does not count as a token
	char * origLine=line;
	int lp=0;
	while ((line[lp])!=0x00) {
		while(isspace((int)line[lp])) lp++;
		if (line[lp]==';') return ntokes;
		if (ntokes==0) line=strdup(origLine); // Make a copy in memory of the original
		tokenPos[ntokes]=lp;
		tokens[ntokes++]=line+lp;
		while(isalnum((int)line[lp]) || line[lp]=='#' || line[lp]=='-') lp++;
		char rep=line[lp];
		if (rep==0x00) return ntokes;
		line[lp]=0x00; // Mark end of token
		lp++;
		int ws=0;
		if (isspace(rep)) {
			while(isspace((int)line[lp])) lp++;
			rep=line[lp];
			ws=1;
		}
		if (rep==';') return ntokes;
		if (line[lp]==',') lp++; // Skip over comma
		if ((!ws) && (rep!=',')) printEmsg(origLine,lnum,lp,"Invalid delimiter found");
	}
	return ntokes;
}

/*---------------------------------------------------------
  printEmsg writes an error message, per the parameters
---------------------------------------------------------*/
void printEmsg(char *line,int lnum,int col,char *txt) {
	printf("Error - %s on line %d.%d\n",txt,lnum,col+1);
	printf("  %3d | %s\n",lnum,line);
	printf("      |");
	for(int i=0;i<=col;i++) printf(" ");
	printf("^\n");
}

/*---------------------------------------------------------
  strUpper converts a string to all upper case
---------------------------------------------------------*/
void strUpper(char * string) {
	while((*string)!=0x00) {
		(*string)=toupper(*string);
		string++;
	}
}

/*---------------------------------------------------------
  getRegister converts a token Rxx into the int xx value
  		If token is not of the form Rxx, returns -1
---------------------------------------------------------*/
int getRegister(char * string) {
	int reg=-1;
	sscanf(string,"R%d",&reg);
	return reg;
}

/*---------------------------------------------------------
  getImmediate converts a token from #xx to the int xx value
  		If token is not of the form #xx, returns -1
  ---------------------------------------------------------*/
int getImmediate(char *string) {
	int imm=INT_MIN;
	sscanf(string,"#%d",&imm);
	return imm;
}

/*---------------------------------------------------------
  makeInstruction converts the parameters into a 32 bit binary APEX
  instruction, and returns the result.
---------------------------------------------------------*/
int makeInstruction(unsigned char opNum,enum opFormat_enum  format,int dr,int sr1,int sr2,int imm,int offset) {
	int inst=0;
	int mask16=0xffff;
	int mask24=0xffffff;
	// put opcode in 39-24
	inst |= (opNum<<24);

	switch(format) {
		case fmt_nop: break;
		case fmt_dss: inst |= (dr<<20)  | (sr1<<16) | (sr2<<12); break;
		case fmt_dsi: inst |= (dr<<20)  | (sr1<<16) | (imm&mask16); break;
	   case fmt_di:  inst |= (dr<<20)                   | (imm&mask16); break;
	   case fmt_ssi: inst |= (sr2<<20) | (sr1<<16) | (imm&mask16); break;
		case fmt_ss:  inst |= (sr1<<16)                   | (sr2<<12); break;
	   case fmt_off: inst |=                                   (offset&mask24); break;
	   default : printf("Error: format %d not recognized!\n",format);
	}
	return inst;
}