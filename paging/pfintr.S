/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
        popl pferrcode # Pop error code from stack
        pushfl         # Push flag registers
        cli            # Clear interrupts
        pushal         # Push all registers
        call pfint     # Sys Call
        popal          # Pop all registers
        popfl          # Pop all flags
        iret           # Return

