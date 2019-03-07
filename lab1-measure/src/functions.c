/*
  Functions.c

  Ingo Sander, 2005-10-04
  Johan Wennlund, 2008-09-19
  George Ungureanu, 2018-08-27

*/

#include <stdio.h>
#include "system.h"
#include <time.h>
#include <sys/alt_timestamp.h>
#include "alt_types.h"

#define M 64

int matrix[M][M]; 

/* Initialize the matrix */

void initMatrix (int matrix[][M]);
int  sumMatrix  (int matrix[][M], int size);

alt_u32 ticks;
alt_u32 time_1;
alt_u32 time_2;
alt_u32 timer_overhead;

float microseconds(int ticks)
{
  return (float) 1000000 * (float) ticks / (float) alt_timestamp_freq();
}

void start_measurement()
{
  alt_timestamp_start();
  time_1 = alt_timestamp();
}

void stop_measurement()
{
  time_2 = alt_timestamp();
  ticks = time_2 - time_1;
}

int main ()
{
  
  int a;
  
  printf("Processor Type: %s\n", NIOS2_CPU_IMPLEMENTATION);

  /* Check if timer available */
  if (alt_timestamp_start() < 0)
    printf("No timestamp device available!");
  else
    {
      /* Print frequency and period */
      printf("Timestamp frequency: %3.1f MHz\n", (float)alt_timestamp_freq()/1000000.0);
      printf("Timestamp period:    %f ms\n\n", 1000.0/(float)alt_timestamp_freq());  

      /* Calculate Timer Overhead */
      // Average of 10 measurements */
      int i;
      timer_overhead = 0;
      for (i = 0; i < 10; i++) {      
        start_measurement();
        stop_measurement();
        timer_overhead = timer_overhead + time_2 - time_1;
      }
      timer_overhead = timer_overhead / 10;
        
      printf("Timer overhead in ticks: %d\n", (int) timer_overhead);
      printf("Timer overhead in ms:    %f\n\n", 
	     1000.0 * (float)timer_overhead/(float)alt_timestamp_freq());
    
      printf("Measuring sumMatrix...");
      initMatrix(matrix);     
      start_measurement();
      a = sumMatrix (matrix, M);
      stop_measurement();    
      printf("Result: %d\n", a);
      printf("%5.2f us", microseconds(ticks - timer_overhead));
      printf("(%d ticks)\n", (int) (ticks - timer_overhead)); 

      printf("Done!\n");

    }    
  return 0;
}

void initMatrix (int matrix[M][M]){
  int i, j;

  for (i = 0; i < M; i++) {
    for (j = 0; j < M; j++) {
      matrix[i][j] = i+j;
    }
  }
}

int sumMatrix (int matrix[M][M], int size)
{
  int i, j, Sum = 0;

  for (i = 0; i < size; i ++) {
    for (j = 0; j < size; j ++) {
      Sum += matrix[i][j];
    }
  }
  return Sum;
}
