MOVC R1,#10 ; Initialize the countdown
MOVC R2,#0 ; Memory location to write the first value
ADDL R1,R1,#0 ; Set the condition code based on R1 value
BP #8 ; Skip over halt instruction if R1>0
HALT
STORE R1,R2,#0 ; Store the current countdown value
ADDL R2,R2,#4 ; Move to the next memory location
ADDL R1,R1,#-1 ; Subtract 1 from R1
JUMP #-20; Go back to the BZ instruction  CC from prev ADDL
NOP; Needed to fill in pipe to keep going