#include <stdio.h>  /* Include definition of putchar. */

/*
 * Function prototype for hexasc, called from this file.
 */
int hexasc( int );

/*
 * puttime - read time from memory and print on console
 * 
 * Parameter (only one): the address of the time variable.
 */
void puttime( int * timeloc )
{
  /* Read time variable. */
  int tmp = * timeloc;
  
  /* Send Newline character. */
  putchar( '\n' );

  /* Send time to console. */
  putchar( hexasc( tmp >> 12 ) ); /* First digit */
  putchar( hexasc( tmp >>  8 ) ); /* Second digit */
  putchar( ':' );                 /* Colon */
  putchar( hexasc( tmp >>  4 ) ); /* Third digit */
  putchar( hexasc( tmp       ) ); /* Last digit */
}
