        # Lab program for Nios-II IDE tutorial
        # Created 2007-10-24 by F Lundevall
        # Copyright abandoned. This file is in the public domain.
        
        .text                   # Instructions follow

        .global hexasc          # makes label "main" globally known

hexasc: andi    r4,r4,0xF       # only consider lower 4 bits

        movi    r8,0xA          # #10
        bge     r4,r8,AtoF      # if the input is in 10-15 range
        
        movi    r8,0x30         # ascii code for '0'        
        br      fin
AtoF:        
        movi    r8,0x41-0xA     # ascii code for 'A'
fin:
        add     r2,r4,r8        # ascii code for input
        
        ret

#        .end                    # The assembler will stop here
