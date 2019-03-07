// File: TwoTasks.c 

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


OS_EVENT *SemW;
OS_EVENT *SemR;

OS_MEM *ShMem;
INT32S ShValue;
/* Definition of Task Priorities */
#define TASK1_PRIORITY      6  // highest priority
#define TASK2_PRIORITY      7
#define TASK_STAT_PRIORITY 12  // lowest priority 

INT8U err;

/* Prints a message and sleeps for given time interval */
void task1(void* pdata)
{
	INT32S *value; //pointer to the shmem
	INT32S temp=0; //value displayed
	while(1){
	
	temp++; //we first increase the value
	printf("Sending %i\n", temp); //print
	*value = temp; //the pointer points to the displayed value
	err= OSMemPut(ShMem, (void *) value); //we put the pointer on the shmem
	OSSemPost(SemR); //CS
	OSSemPend(SemW, 0,&err); //Return to the function
	value=(INT32S *) OSMemGet(ShMem,&err); //We get back the new value
	printf("Received %i\n", *value); 
	}
}

/* Prints a message and sleeps for given time interval */
void task2(void* pdata)
{
 	INT32S *value; 
	INT32S temp;
	while(1){
		OSSemPend(SemR, 0,&err); 
		value=(INT32S *) OSMemGet(ShMem,&err); // get the address 
		temp= *value;  
		*value = -temp; //change of the value
		err= OSMemPut(ShMem, (void *) value); //send it back
		OSSemPost(SemW);
	}
}


/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 3 - Two Tasks\n");

SemR= OSSemCreate(0);
SemW= OSSemCreate(0);
ShMem= OSMemCreate(&ShValue, 1, sizeof(INT32S),&err);

}
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
