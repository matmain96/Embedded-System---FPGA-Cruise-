#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"

extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);

#define TRUE 1

int timeloc = 0x5957; /* startvalue given in hexadecimal/BCD-code */
short run = 1;

void pollkey()
{
    int btn_reg;
    static int last_value = -1;
    btn_reg = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_KEYS4_BASE);
    btn_reg = (~btn_reg) & 0xf;
    if (last_value != btn_reg)
    {
        last_value = btn_reg;
        switch (btn_reg)
        {
            case 1:
                run = 1;
                break;
            case 2:
                run = 0;
                break;
            case 4:
                tick (&timeloc);
                break;
            case 8:
                timeloc = 0;
                break;
            default:
                break;
        }
    }
}

int main ()
{
    int i;    
    while (TRUE)
    {
        if (run)
            tick (&timeloc);
        puttime (&timeloc);
        for (i=0;i<1000;i++)
        {
            IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc);
            puthex(timeloc);
            delay (1);
            pollkey();
        }
    }
    
    return 0;
}
