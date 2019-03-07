#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_alarm.h"
#include "alt_types.h"

#include "next_prime.h"

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
    btn_reg = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_KEYS4_BASE);
    btn_reg = (~btn_reg) & 0xf;
    switch (btn_reg)
    {
        case 1:
            run = !run;
            break;
        case 2:
            tick (&timeloc);
            break;
        case 4:
            timeloc = 0x0000;
            break;
        case 8:
            timeloc = 0x5957;
            break;
        default:
            break;
    }
}

// The interrupt service routine
static void Key_InterruptHandler(void* context, alt_u32 id)
{ 
    pollkey();
    /* Write to the edge capture register to reset it. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(DE2_PIO_KEYS4_BASE, 0);
    /* reset interrupt capability for the Button PIO. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(DE2_PIO_KEYS4_BASE, 0xf);
}

alt_u32 show (void* context)
{
  /* This function will be called once/second */
  if (run)
    tick (&timeloc);
  puttime (&timeloc);
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc);
  puthex(timeloc);  
  return alt_ticks_per_second();
}

int main ()
{
    /* set interrupt capability for the Button PIO. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(DE2_PIO_KEYS4_BASE, 0xf);
     /* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(DE2_PIO_KEYS4_BASE, 0x0);
    // Register the ISR for buttons
    alt_irq_register(DE2_PIO_KEYS4_IRQ, NULL, Key_InterruptHandler);
    
    /* The alarm for calling the timer function */
    static alt_alarm alarm;  
    /* Register the flashing function for the timer */
    if (alt_alarm_start (&alarm,alt_ticks_per_second(),show,NULL) < 0)
    {
        printf ("No system clock available\n");
    }
    
    int current_prime = 0;
    while (TRUE)
    {
        current_prime = next_prime(current_prime);
        printf("\nNext Prime is %d",current_prime);
    }
    
    return 0;
}
