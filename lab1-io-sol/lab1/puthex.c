#include "system.h"
#include "altera_avalon_pio_regs.h"

static int b2sLUT[] = {0x40,
                 0x79,
                 0x24,
                 0x30,
                 0x19,
                 0x12,
                 0x02,
                 0x78,
                 0x00,
                 0x18,
                 0x08,
                 0x03,
                 0x27,
                 0x21,
                 0x06,
                 0x0E};

int bcd2seven(int inval)
{
    return b2sLUT[inval];
}

/*
 * puthex - 
 * 
 * Parameter (only one): the time variable.
 */
void puthex( int inval )
{
  /* The return value. */
  int tmp = 0;

  /* Send time to console. */
  tmp = ( bcd2seven( (inval & 0xf000) >> 12 ) << 21) | /* First digit */
        ( bcd2seven( (inval & 0x0f00) >>  8 ) << 14) | /* Second digit */
        ( bcd2seven( (inval & 0x00f0) >>  4 ) <<  7) | /* Third digit */
        ( bcd2seven( (inval & 0x000f)       )      );  /* Last digit */
  //tmp = tmp1 |tmp2|tmp3|tmp4;
  
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,tmp);
}
