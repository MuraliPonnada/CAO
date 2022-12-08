MOVC R0,#320
MOVC R1,#96
CMP  R1,R0
BP   #16 ; if r1<=r0 switch r1/r0
ADDL R2,R1,#0
ADDL R1,R0,#0
ADDL R0,R2,#0
SUB  R1,R1,R0 ; get here if r1>r0.. update r1=r1-r0
BNP  #8 ; If the result is zero, we are done
JUMP #-28 ; go back and compare again
MOVC R2,#0
STORE R0,R2,#0
HALT