CC = gcc
CFLAGS = -Wall -std=c18 -ggdb
PGM = example

test : apexSim ${PGM}.o
	./apexSim ${PGM}.o
	
gdb : apexSim ${PGM}.o
	gdb apexSim
	
apexSim : apexSim.o apexCPU.o	apexMem.o apexOpcodes.o

apexOpcodes.o : apexOpcodes.c apexOpcodes.h apexCPU.h apexMem.h

apexSim.o : apexSim.c apexCPU.h apexOpcodes.h

apexCPU.o : apexCPU.c apexCPU.h apexOpcodes.h apexMem.h

apexMem.o : apexMem.c apexMem.h apexCPU.h apexOpcodes.h 

${PGM}.o : apexAsm ${PGM}.s
	./apexAsm ${PGM}.s
	
gdbAsm : apexAsm
	gdb -ex "b main" -ex "run ${PGM}.s" ./apexAsm
	
apexAsm : apexAsm.c apexOpcodes.h apexOpInfo.h
	${CC} ${CFLAGS} -o apexAsm apexAsm.c

clean : 
	-rm apexAsm apexSim *.o