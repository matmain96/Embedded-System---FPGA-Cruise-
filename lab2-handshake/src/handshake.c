#include <stdio.h>
#include "includes.h"
#include <string.h>

#define DEBUG 0

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    stat_stk[TASK_STACKSIZE];

OS_EVENT *sem0;
OS_EVENT *sem1;
INT8U err;
/* Definition of Task Priorities */
#define TASK1_PRIORITY      6  // highest priority
#define TASK2_PRIORITY      7
#define TASK_STAT_PRIORITY 12  // lowest priority


/* Prints a message and sleeps for given time interval */
void task1(void* pdata)
{

    int value=0;
    while(1){   

        OSSemPend(sem1,0,&err); //Wait for the sem
        printf("Task 1 - state %d\n", value);
        value=1;
        printf("Task 1 - state %d\n", value);
        OSSemPost(sem0); //increase other sem. to unlock the other task
        OSSemPend(sem1,0,&err); //Wait for other task to switch context
        value=0;
        printf("Task 1 - state %d\n\n\n", value);
        OSSemPost(sem0);
    }

}

/* Prints a message and sleeps for given time interval */
void task2(void* pdata)
{   
    int value=0;
    while(1){   

        OSSemPend(sem0,0,&err); //Semaphore, the beginning value is one
        printf("Task 0 - state %d\n", value); 
        value=1; 
        OSSemPost(sem1); //increase semaphore
        OSSemPend(sem0,0,&err); //context switch
        printf("Task 0 - state %d\n", value); 
        value=0; 
        printf("Task 0 - state %d\n", value); 
        OSSemPost(sem1); //increase value and start again
    }
}



/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 3 - Two Tasks\n");
  sem0= OSSemCreate(1);
  sem1= OSSemCreate(0);

 
   
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

  OSStart();
  return 0;
}


