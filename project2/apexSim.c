#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "apexCPU.h"

void simCommands(cpu cpu);

int main(int argc, char **argv) {
	setbuf(stdout,0);
	struct apexCPU_struct apexCPU;
	int posArg=1;
	if (argc>posArg) {
		if (0==strcmp(argv[posArg],"-h") || 0==strcmp(argv[posArg],"?")) {
			printf("APEX Simulator\n");
			printf("Invoke as: %s [objectFileName]\n",argv[0]);
			printf("If [objectFileName] is specified, it will be loaded in the simulator.\n");
			printf("Once started, the simulator will prompt for simulator commands with \"APEXSIM ==>\"\n");
			printf("Enter the command \"help\" for information on simulator commands\n");
			posArg++;
		}
	}

	initCPU(&apexCPU);
	if (argc>posArg) loadCPU(&apexCPU,argv[posArg]);
	simCommands(&apexCPU);
	printStats(&apexCPU);
	return 0;
}

void simCommands(cpu cpu) {
	// prompt and execute APEX CPU simulation commands
	char cmdBuf[128],prevCmd[128];
	int verbose=0;
	int done=0;
	prevCmd[0]=0x00;
	while(!feof(stdin) && !done) {
		printf("APEXsim==> ");
		if (NULL==fgets(cmdBuf,127,stdin)) {
			if (feof(stdin)) continue;
			perror("Error - reading apex simulation command:");
			break;
		}
		// Strip trailing newline if there
		int ll=strlen(cmdBuf);
		if (cmdBuf[ll-1]=='\n') cmdBuf[ll-1]='\0';
		char * bufPtr=cmdBuf;
		while(isspace((int)(*bufPtr))) bufPtr++;
		if (strlen(bufPtr)==0) bufPtr=prevCmd;
		else strcpy(prevCmd,bufPtr);

		switch(bufPtr[0]) {
			case 'q':
				done=1;
				continue;
			case 'l':
				bufPtr++;
				if (bufPtr[0]=='o') {
					bufPtr++;
					if (bufPtr[0]=='a') {
						bufPtr++;
						if (bufPtr[0]=='d') {
							bufPtr++;
						}
					}
				}
				if (!isspace((int)bufPtr[0])) {
					printf("expected load <objfile>. Got %s\n",cmdBuf);
					continue;
				}
				while(isspace((int)bufPtr[0])) bufPtr++; // Skip spaces after load
				loadCPU(cpu,bufPtr);
				continue;
			case 'h':
			case '?':
				printf("\n  APEX simulation commands...\n");
				printf("      quit - to exit simulation\n");
				printf("      help or ? - to print this help\n");
				printf("      load <objfilename> - to load the object file into memory and reset simulation\n");
				printf("      cycle - to simulate a single cycle\n");
				printf("      run - to repeat cycles until HALT is retired or abnormal termination\n");
				printf("      verbose - toggle automatic invocation of  \"state\" after each cycle (starts off)\n");
				printf("      state - to print current state of APEX registers\n");
				printf("      <empty> - repeat previous command\n");
				printf("All commands can be abbreviated to one letter.\n");
				continue;
			case 's':
				printState(cpu);
				continue;
			case 'v':
				verbose=!verbose;
				continue;
			case 'c':
				cycleCPU(cpu);
				if (verbose) printState(cpu);
				continue;
			case 'r': {
				int maxCycles=20;
				while((cpu->stop==0) & (maxCycles>0)) {
					cycleCPU(cpu);
					if (verbose) printState(cpu);
					maxCycles--;
				}
				if (maxCycles<=0) {
					printf("... stopped after 20 cycles... use \"run\" again to continue\n");
				}
				continue;
			}
			default:
				printf("Unrecognized APEX simulation command: %s\n",cmdBuf);
		}
	}
}