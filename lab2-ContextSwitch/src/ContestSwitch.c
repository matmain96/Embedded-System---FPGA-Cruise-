// File: TwoTasks.c

#include <stdio.h>
#include "includes.h"
#include <string.h>
#include <time.h>
#include <sys/alt_timestamp.h>
#include "alt_types.h"


/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];
//OS_STK    stat_stk[TASK_STACKSIZE];

INT8U state = 0;

/* Definition of Task Priorities */
#define TASK1_PRIORITY      12
#define TASK2_PRIORITY      7
#define TASK3_PRIORITY      6

// Declaration of semaphores
OS_EVENT *SemT1;
OS_EVENT *SemT2;
OS_EVENT *SemT3;
// Error variable, wich holds a value if an error event is ocurred
INT8U err;

alt_u32 ticks;
alt_u32 time_1;
alt_u32 time_2;
alt_u32 timer_overhead;

 /* Get microseconds from clock ticks */
float microseconds(int ticks)
{
  return (float) 1000000 * (float) ticks / (float) alt_timestamp_freq();
}
 /* Start measurement of time */
void start_measurement()
{
  alt_timestamp_start();
  time_1 = alt_timestamp();
}
 /* Stop measurement of time */
void stop_measurement()
{
  time_2 = alt_timestamp();
  ticks = time_2 - time_1;
}
/* Prints the initial state of Task 1 and inverts the state. Finally, it prints the final state and post semaphore T2 */
// Starts a counter
void task1(void* pdata)
{
  OSSemPost(SemT1);
  while (1)
    {
      OSSemPend(SemT1,0,&err);
      printf("Task 1 - State %i\n", state);
      if (state == 0) state = 1;
      else state = 0;
      printf("Task 1 - State %i\n", state);
      start_measurement();
      OSSemPost(SemT2);

      OSTimeDlyHMSM(0, 0, 0, 10);
        /* Context Switch to next task
                   * Task will go to the ready state
                   * after the specified delay
                   */
    }
}

/* Prints the initial state of T2 (final state of T1) and inverts the state. Finally, it prints the final state and post semaphore T1.  */
// Calculates t
void task2(void* pdata)
{
  while (1)
    {
     OSSemPend(SemT2,0,&err);
     stop_measurement();
     printf("Task 2 - State %i\n", state);
     if (state == 0) state = 1;
     else state = 0;
     printf("Task 2 - State %i\n\n", state);
     OSSemPost(SemT3);
     OSTimeDlyHMSM(0, 0, 0, 10);
    }
}
/* Print of Context Switch time and average every 10 measurements */
void task3(void* pdata)
{
  int i = 0;
  float time_acum = 0;
  float time_avg;

  while (1)
    {
     OSSemPend(SemT3,0,&err);
     if (i<10){
        i++;
        printf("Context Swicth measurement (us): %f \nMeasurement number: %i\n\n", microseconds(ticks), i);
        time_acum = time_acum + microseconds(ticks);
     }
     else{
         time_avg = time_acum/i;
        printf("Context Switch avg time: %f\n", time_avg);
        printf("Measurements taken: %i\n",i);
        i=0;
        time_acum=0;
     }
     OSSemPost(SemT1);
     OSTimeDlyHMSM(0, 0, 1, 100);
    }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 3 - Context Switch measurement\n\n");
  SemT1=OSSemCreate(0);
  SemT2=OSSemCreate(0);
  SemT3=OSSemCreate(0);

  OSTaskCreateExt
    ( task1,                        // Pointer to task code
      NULL,                         // Pointer to argument passed to task
      &task1_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
      TASK1_PRIORITY,               // Desired Task priority
      TASK1_PRIORITY,               // Task ID
      &task1_stk[0],                // Pointer to bottom of task stack
      TASK_STACKSIZE,               // Stacksize
      NULL,                         // Pointer to user supplied memory (not needed)
      OS_TASK_OPT_STK_CHK |         // Stack Checking enabled
      OS_TASK_OPT_STK_CLR           // Stack Cleared                                 
      );
       
  OSTaskCreateExt
    ( task2,                        // Pointer to task code
      NULL,                         // Pointer to argument passed to task
      &task2_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
      TASK2_PRIORITY,               // Desired Task priority
      TASK2_PRIORITY,               // Task ID
      &task2_stk[0],                // Pointer to bottom of task stack
      TASK_STACKSIZE,               // Stacksize
      NULL,                         // Pointer to user supplied memory (not needed)
      OS_TASK_OPT_STK_CHK |         // Stack Checking enabled
      OS_TASK_OPT_STK_CLR           // Stack Cleared                       
      );
 
  OSTaskCreateExt
    ( task3,                        // Pointer to task code
      NULL,                         // Pointer to argument passed to task
      &task3_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
      TASK3_PRIORITY,               // Desired Task priority
      TASK3_PRIORITY,               // Task ID
      &task3_stk[0],                // Pointer to bottom of task stack
      TASK_STACKSIZE,               // Stacksize
      NULL,                         // Pointer to user supplied memory (not needed)
      OS_TASK_OPT_STK_CHK |         // Stack Checking enabled
      OS_TASK_OPT_STK_CLR           // Stack Cleared                                 
      );

  OSStart();
  return 0;
}
