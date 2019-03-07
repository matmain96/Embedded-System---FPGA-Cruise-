
/*
 * tick - update the time by adding one second
 * 
 * Parameter (only one): the address of the time variable.
 */
void tick( int * timeloc )
{
  /* Read time variable. */
  int tmp = * timeloc;
  
  tmp = tmp + 1; /* try a tick */
  
  /* if we ticked from 9 seconds to 10, adjust time properly */
  if( (tmp & 0x000f) == 0x000a ) tmp = tmp - 0x000a + 0x0010;
  
  /* if we ticked from 59 seconds to 60, adjust time properly */
  if( (tmp & 0x00f0) == 0x0060 ) tmp = tmp - 0x0060 + 0x0100;
  
  /* if we ticked from 9 minutes to 10, adjust time properly */
  if( (tmp & 0x0f00) == 0x0a00 ) tmp = tmp - 0x0a00 + 0x1000;
  
  /* if we ticked from 59 minutes to 60, adjust time properly */
  if( (tmp & 0xf000) == 0x6000 ) tmp = 0x0000;
  
  *timeloc = tmp;      /* update memory with new time value */
}
