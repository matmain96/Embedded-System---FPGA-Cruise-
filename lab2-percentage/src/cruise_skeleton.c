#include <stdio.h>
#include "system.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"

#define DEBUG 1
#define HW_TIMER_PERIOD 100 /* 100ms */

/* Button Patterns */

#define GAS_PEDAL_FLAG      0x08
#define BRAKE_PEDAL_FLAG    0x04
#define CRUISE_CONTROL_FLAG 0x02
/* Switch Patterns */

#define TOP_GEAR_FLAG       0x00000002
#define ENGINE_FLAG         0x00000001
/* LED Patterns */

#define LED_RED_0 0x00000001 // Engine
#define LED_RED_1 0x00000002 // Top Gear

#define LED_GREEN_0 0x0001 // Cruise Control activated
#define LED_GREEN_2 0x0002 // Cruise Control Button
#define LED_GREEN_4 0x0010 // Brake Pedal
#define LED_GREEN_6 0x0040 // Gas Pedal

/*
 * Definition of Tasks
 */

#define TASK_STACKSIZE 2048

OS_STK StartTask_Stack[TASK_STACKSIZE]; 
OS_STK ControlTask_Stack[TASK_STACKSIZE]; 
OS_STK VehicleTask_Stack[TASK_STACKSIZE];
OS_STK SwitchIO_Stack[TASK_STACKSIZE]; 
OS_STK ButtonIO_Stack[TASK_STACKSIZE]; 
OS_STK Watchdog_Stack[TASK_STACKSIZE];	
OS_STK Overload_Stack[TASK_STACKSIZE];	
OS_STK Checker_Stack[TASK_STACKSIZE];
// Task Priorities
 
#define STARTTASK_PRIO     4
#define VEHICLETASK_PRIO  8
#define CONTROLTASK_PRIO  10
#define SWITCH_PRIO 12
#define BUTTON_PRIO 14
#define WATCHDOG_PRIO 6
#define OVERLOAD_PRIO 16
#define CHECKER_PRIO 17
// Task Periods

#define CONTROL_PERIOD  50 //0.1s
#define VEHICLE_PERIOD  50 //0.1s
#define SWITCH_PERIOD 30
#define BUTTONS_PERIOD 30
#define WATCHDOG_PERIOD 150
#define OVERLOAD_PERIOD 10
/*
 * Definition of Kernel Objects 
 */

// Mailboxes
OS_EVENT *Mbox_Throttle;
OS_EVENT *Mbox_Velocity;
//Semaphores

OS_EVENT *SemControl;
OS_EVENT *SemVehicle;
OS_EVENT *SemCar;
OS_EVENT *SemButtons;
OS_EVENT *SemWatchdog;
OS_EVENT *SemOverload;

// SW-Timer

OS_TMR *TmrVehicle;
OS_TMR *TmrControl;
OS_TMR *TmrOnoff;
OS_TMR *TmrButtons;
OS_TMR *TmrWatchdog;
OS_TMR *TmrOverload;
/*
 * Types
 */
enum active {on, off};

enum active gas_pedal = off;
enum active brake_pedal = off;
enum active top_gear = off;
enum active engine = off;
enum active cruise_control = off; 


enum status {ok, overloaded};

enum status check= ok;
/*
 * Global variables
 */
int delay; // Delay of HW-timer 
INT16U led_green = 0; // Green LEDs
INT32U led_red = 0;   // Red LEDs

/*
These handlers goes on when the software timers finish their period
	They unlock the relative task increasing the value of their semaphore
*/
void VehicleHandler(void *ptmr, void* callback_arg){
		OSSemPost(SemVehicle);
}
void ControlHandler(void *ptmr, void* callback_arg){
		OSSemPost(SemControl);
}
void CarHandler(void *ptmr, void* callback_arg){
		OSSemPost(SemCar);
}
void WatchdogHandler(void *ptmr, void* callback_arg){
		
		OSSemPost(SemWatchdog);
}
void OverloadHandler(void *ptmr,void* callback_arg){
		OSSemPost(SemOverload);
}
void ButtonsHandler(void *ptmr,void* callback_arg){
		OSSemPost(SemButtons);
}
int buttons_pressed(void)
{
  return ~IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_KEYS4_BASE)&15;    
}

int switches_pressed(void)
{
  return IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_TOGGLES18_BASE);    
}

/*
 * ISR for HW Timer
 */
alt_u32 alarm_handler(void* context)
{
  OSTmrSignal(); /* Signals a 'tick' to the SW timers */
  
  return delay;
}

static int b2sLUT[] = {0x40, //0
		       0x79, //1
		       0x24, //2
		       0x30, //3
		       0x19, //4
		       0x12, //5
		       0x02, //6
		       0x78, //7
		       0x00, //8
		       0x18, //9
		       0x3F, //-
};

/*
 * convert int to seven segment display format
 */
int int2seven(int inval){
  return b2sLUT[inval];
}

/*
 * output current velocity on the seven segement display
 */
void show_velocity_on_sevenseg(INT8S velocity){
  int tmp = velocity;
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;
  INT8U out_sign = 0;

  if(velocity < 0){
    out_sign = int2seven(10);
    tmp *= -1;
  }else{
    out_sign = int2seven(0);
  }

  out_high = int2seven(tmp / 10);
  out_low = int2seven(tmp - (tmp/10) * 10);
  
  out = int2seven(0) << 21 |
    out_sign << 14 |
    out_high << 7  |
    out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,out);
}

/*
 * shows the target velocity on the seven segment display (HEX5, HEX4)
 * when the cruise control is activated (0 otherwise)
 */
void show_target_velocity(INT8U target_vel)
{
  
  int tmp = target_vel;
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;

  if(cruise_control==on){
   out_high = int2seven(tmp / 10);
   out_low = int2seven(tmp - (tmp/10) * 10);
  }else{
	out_high= int2seven(0);
	out_low= int2seven(0);
  }
  
  out = int2seven(0) << 21 |
  int2seven(0) << 14 |
  out_high << 7  |
  out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_HIGH28_BASE,out);
	
}


	
/*
 * indicates the position of the vehicle on the track with the four leftmost red LEDs
 * LEDR17: [0m, 400m)
 * LEDR16: [400m, 800m)
 * LEDR15: [800m, 1200m)
 * LEDR14: [1200m, 1600m)
 * LEDR13: [1600m, 2000m)
 * LEDR12: [2000m, 2400m]
 */
void show_position(INT16U position){
	
	if(position==24000){
			/*if the position is the max available, instead of coming back to zero we turn the last led on*/
		 IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,(	(1<<12)	|	(switches_pressed()&3)	)	); 
	}
	else{	
		position=position%24000; //to receive the right value
		switch(position/4000){ //we divide to know which led turn on

	/*		We turn on the desired led
		we also need to keep the other leds on, that why we use the Bitwise operation		*/
			case 0: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<17)|(switches_pressed()&3))); 
					break;
			case 1: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<16)|(switches_pressed()&3)));
					break;
			case 2: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<15)|(switches_pressed()&3)));
					break;
			case 3: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<14)|(switches_pressed()&3)));
					break;
			case 4: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<13)|(switches_pressed()&3)));
					break;
			case 5: IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,((1<<12)|(switches_pressed()&3)));
					break;
		}
	}
}

/*
 * The function 'adjust_position()' adjusts the position depending on the
 * acceleration and velocity.
 */
INT16U adjust_position(INT16U position, INT16S velocity,
		       INT8S acceleration, INT16U time_interval)
{
  INT16S new_position = position + velocity * time_interval / 1000
    + acceleration / 2  * (time_interval / 1000) * (time_interval / 1000);

  if (new_position > 24000) {
    new_position -= 24000;
  } else if (new_position < 0){
    new_position += 24000;
  }
  
  show_position(new_position);
  return new_position;
}
 
/*
 * The function 'adjust_velocity()' adjusts the velocity depending on the
 * acceleration.
 */
INT16S adjust_velocity(INT16S velocity, INT8S acceleration,  
		       enum active brake_pedal, INT16U time_interval)
{
  INT16S new_velocity;
  INT8U brake_retardation = 200;
  if(cruise_control==on) //if the cruise control is on the speed does not change
		return velocity;
  if (brake_pedal == off)
    new_velocity = velocity  + (float) (acceleration * time_interval) / 1000.0;
  else {
    if (brake_retardation * time_interval / 1000 > velocity)
      new_velocity = 0;
    else
      new_velocity = velocity - brake_retardation * time_interval / 1000;
  }

  return new_velocity;
}

void ButtonIO(void* pdata){
	int state2=0;
	int state4=0;
	int state8=0;
	INT8U err;
	printf("Button Task Generated!\n");
	OSTmrStart(TmrButtons,&err);
	while(1){
		OSSemPend(SemButtons,0,&err);
		switch(buttons_pressed()){
		
			case 0x02:	
						if(state2!=1){	
							state2=1;	//control value
							/* if the cruise control is on, it get turned off */
							if((IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE)&0x0004)!=0x0004){
							/* if the cruise control is off, we prepare it to get on, at the desired conditions*/
								if((IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE!=1))){ 
									/* meanwhile we activate the ledg2 */
									IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0004);
									brake_pedal=off;
									gas_pedal=off;
								}
								else {
										cruise_control=off;
										brake_pedal=off;
										gas_pedal=off;
									}
								}
							else{
							 IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0000);
							  	
							 cruise_control=off;
								 }
						}	
						
						break;
			case 0x04: //brake
						if(state4!=1){
							state4=1;
							/* in both cases, we check if the lights is on, if it is on the switch it off */
							if(IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE) != 0x0010){
								IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0010);
								brake_pedal= on;
								gas_pedal=off;
								cruise_control=off;
								
							}
							else {
								IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0000);
								brake_pedal=off;
							}
						}
						break;
			case 0x08: 
					if(state8!=1){
						state8=1;
						if((IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE)) != 0x0040){
							IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0040);
							gas_pedal=on;
							brake_pedal=off;
							cruise_control=off;
						}
						else {
						IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0x0000);
						gas_pedal=off;
						}
					}
					break;
		default:
				state2=0;
				state4=0;
				state8=0;
				break;
		}	



	}
	
}

void SwitchIO(void* pdata){
	INT8U err;
	printf("switch Task Generated!\n");
	BOOLEAN status;
	status = OSTmrStart(TmrOnoff,&err);	
	
	while(1){
		OSSemPend(SemCar,0,&err); //pause
		/* We write on the redled the precedent set of lights, plus the switches value */
		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,(IORD_ALTERA_AVALON_PIO_DATA( ((DE2_PIO_REDLED18_BASE)&(~3)) | (switches_pressed()&3)	)));
		//based on the switches value, we turn on the relative values
		switch(switches_pressed()&3){
			case 0:	engine=off;
					top_gear=off;	
					cruise_control=off;
					break;
			case 1:	engine=on;
					top_gear=off;
					cruise_control=off;
					break;
			case 2:	
					engine=off;
					top_gear=on;
					
					break;
			case 3: engine=on;
					top_gear=on;
					break;
		}
	}	
}

/*
 * The task 'VehicleTask' updates the current velocity of the vehicle
 */

void Watchdog(void* pdata)
{	
	
	INT8U err;
	OSTmrStart(TmrWatchdog,&err);
	while(1){

	if(check == 1)	{
		printf("\n =====> CPU USAGE OVERLOAD\n");
	}
	else {
		printf("\n =====> CPU OK\n");
	}
	check= 1;
	OSSemPend(SemWatchdog,0,&err);
	}
}

void Overload(void* pdata){
	INT8U err;
	OSTmrStart(TmrOverload,&err);
	int i;
	int reader;
	int delayer;
	while(1){
		reader= switches_pressed();
		reader= reader>>4;
		reader= reader&0x003f;
		if (reader>50){
			reader=50;
		}
		delayer= (WATCHDOG_PERIOD*delay)/50;
		delayer*= reader;
		OSTimeDlyHMSM(0,0,delayer,0);
	//	OSTmrStart(TmrWatchdog,&err); one of the two
		check=0; 
	}	

}

void Checker(void* pdata){

	while(1) {	
		
		//check=0;
	}
}

void VehicleTask(void* pdata)
{ 
  INT8U err;  
  void* msg;
  INT8U* throttle; 
  INT8S acceleration;  /* Value between 40 and -20 (4.0 m/s^2 and -2.0 m/s^2) */
  INT8S retardation;   /* Value between 20 and -10 (2.0 m/s^2 and -1.0 m/s^2) */
  INT16U position = 0; /* Value between 0 and 20000 (0.0 m and 2000.0 m)  */
  INT16S velocity = 0; /* Value between -200 and 700 (-20.0 m/s amd 70.0 m/s) */
  INT16S wind_factor;   /* Value between -10 and 20 (2.0 m/s^2 and -1.0 m/s^2) */
  BOOLEAN status;
  printf("Vehicle task created!\n");
  status = OSTmrStart(TmrVehicle,&err); 
  while(1)
    {
      err = OSMboxPost(Mbox_Velocity, (void *) &velocity);

      OSSemPend(SemVehicle,0,&err);

      /* Non-blocking read of mailbox: 
	 - message in mailbox: update throttle
	 - no message:         use old throttle
      */
      msg = OSMboxPend(Mbox_Throttle, 1, &err); 
      if (err == OS_NO_ERR) 
	throttle = (INT8U*) msg;

      /* Retardation : Factor of Terrain and Wind Resistance */
      if (velocity > 0)
	wind_factor = velocity * velocity / 10000 + 1;
      else 
	wind_factor = (-1) * velocity * velocity / 10000 + 1;
         
      if (position < 4000) 
	retardation = wind_factor; // even ground
      else if (position < 8000)
	retardation = wind_factor + 15; // traveling uphill
      else if (position < 12000)
	retardation = wind_factor + 25; // traveling steep uphill
      else if (position < 16000)
	retardation = wind_factor; // even ground
      else if (position < 20000)
	retardation = wind_factor - 10; //traveling downhill
      else
	retardation = wind_factor - 5 ; // traveling steep downhill
      
      acceleration = *throttle / 2 - retardation;	  
      position = adjust_position(position, velocity, acceleration, 300); 
      velocity = adjust_velocity(velocity, acceleration, brake_pedal, 300); 
      printf("Position: %dm\n", position / 10);
      printf("Velocity: %4.1fm/s\n", velocity /10.0);
      printf("Throttle: %dV\n", *throttle / 10);
      show_velocity_on_sevenseg((INT8S) (velocity / 10));
	  show_target_velocity((INT8S) (velocity/10));
    }
} 
 
/*
 * The task 'ControlTask' is the main task of the application. It reacts
 * on sensors and generates responses.
 */

void ControlTask(void* pdata)
{
  INT8U err;
  INT8U throttle = 40; /* Value between 0 and 80, which is interpreted as between 0.0V and 8.0V */
  void* msg;
  INT16S* current_velocity;
  BOOLEAN status;
  printf("Control Task created!\n");
  status= OSTmrStart(TmrControl,&err);
  while(1)
    { 

		msg = OSMboxPend(Mbox_Velocity, 0, &err);
	  	  current_velocity = (INT16S*) msg;
	 
	  if(( engine == off ) && (*current_velocity<=0)){
			IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,0);
			IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,0);
			IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_HIGH28_BASE,0xfffffff);
			IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,0xfffffff);
			throttle=0;
			cruise_control=off;
			brake_pedal=off;
			gas_pedal=off;
			top_gear=off;
			err = OSMboxPost(Mbox_Throttle, (void *) &throttle);
		}
		else{

		if((gas_pedal == on) && (throttle <80)) 
				throttle=throttle+5;
			  
		else if((gas_pedal == off)){
			    if(throttle<=40){
					throttle+=5;
				}
				if(throttle>=40){
					throttle-=5;				
				}
			}
		
  		 if( (top_gear == on) && (IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE)==4) && (( *current_velocity <=250) && (*current_velocity>180))){
				cruise_control=on;	
				IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE,1);
		  } 
		  err = OSMboxPost(Mbox_Throttle, (void *) &throttle);
		  OSSemPend(SemControl,0,&err);
		}
	  }
    
}

/* 
 * The task 'StartTask' creates all other tasks kernel objects and
 * deletes itself afterwards.
 */ 

void StartTask(void* pdata)
{
  INT8U err;
  void* context;

  static alt_alarm alarm;     /* Is needed for timer ISR function */
  
  /* Base resolution for SW timer : HW_TIMER_PERIOD ms */
  delay = alt_ticks_per_second() * HW_TIMER_PERIOD / 1000; 
  printf("delay in ticks %d\n", delay);

  /* 
   * Create Hardware Timer with a period of 'delay' 
   */
  if (alt_alarm_start (&alarm,
		       delay,
		       alarm_handler,
		       context) < 0)
    {
      printf("No system clock available!n");
    }

  /* 
   * Create and start Software Timer 
   */

	TmrControl= OSTmrCreate(0,CONTROL_PERIOD,OS_TMR_OPT_PERIODIC,ControlHandler,(void *) 0,"Control",&err);
	TmrVehicle= OSTmrCreate(0,VEHICLE_PERIOD,OS_TMR_OPT_PERIODIC,VehicleHandler,(void *) 0,"Vehicle",&err);
	TmrOnoff= OSTmrCreate(0, SWITCH_PERIOD,OS_TMR_OPT_PERIODIC,CarHandler,(void *) 0, "OnOff",&err);
    TmrWatchdog=OSTmrCreate(0,WATCHDOG_PERIOD,OS_TMR_OPT_PERIODIC,WatchdogHandler,(void *) 0, "Watchdog", &err);
	TmrOverload=OSTmrCreate(0,OVERLOAD_PERIOD,OS_TMR_OPT_PERIODIC,OverloadHandler,(void *) 0, "Overload", &err);
	TmrButtons=OSTmrCreate(0,BUTTONS_PERIOD,OS_TMR_OPT_PERIODIC,ButtonsHandler,(void *) 0, "Overload", &err);
  /*
   * Creation of Kernel Objects
   */
  
  SemControl = OSSemCreate(1);
  SemVehicle = OSSemCreate(1);
  SemCar = OSSemCreate(1);
  SemOverload = OSSemCreate(0);
  SemWatchdog = OSSemCreate(0);
  SemButtons = OSSemCreate(1);
  // Mailboxes
  Mbox_Throttle = OSMboxCreate((void*) 0); /* Empty Mailbox - Throttle */
  Mbox_Velocity = OSMboxCreate((void*) 0); /* Empty Mailbox - Velocity */
 
   
  /*
   * Create statistics task
   */

  OSStatInit();

  /* 
   * Creating Tasks in the system 
   */

  err = OSTaskCreateExt(
			Checker, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&Checker_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			CHECKER_PRIO,
			CHECKER_PRIO,
			(void *)&Checker_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
			Overload, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&Overload_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			OVERLOAD_PRIO,
			OVERLOAD_PRIO,
			(void *)&Overload_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);
  err = OSTaskCreateExt(
			Watchdog, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&Watchdog_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			WATCHDOG_PRIO,
			WATCHDOG_PRIO,
			(void *)&Watchdog_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);


  err = OSTaskCreateExt(
			ButtonIO, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&ButtonIO_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			BUTTON_PRIO,
			BUTTON_PRIO,
			(void *)&ButtonIO_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

    err = OSTaskCreateExt(
			SwitchIO, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&SwitchIO_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			SWITCH_PRIO,
			SWITCH_PRIO,
			(void *)&SwitchIO_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);		


  err = OSTaskCreateExt(
			ControlTask, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&ControlTask_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			CONTROLTASK_PRIO,
			CONTROLTASK_PRIO,
			(void *)&ControlTask_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);
 
  

  err = OSTaskCreateExt(
			VehicleTask, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&VehicleTask_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			VEHICLETASK_PRIO,
			VEHICLETASK_PRIO,
			(void *)&VehicleTask_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

    err = OSTaskCreateExt(
			VehicleTask, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&VehicleTask_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			VEHICLETASK_PRIO,
			VEHICLETASK_PRIO,
			(void *)&VehicleTask_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);
  printf("All Tasks and Kernel Objects generated!\n");

  /* Task deletes itself */

  OSTaskDel(OS_PRIO_SELF);
}

/*
 *
 * The function 'main' creates only a single task 'StartTask' and starts
 * the OS. All other tasks are started from the task 'StartTask'.
 *
 */

int main(void) {

  printf("Lab: Cruise Control\n");
 
  OSTaskCreateExt(
		  StartTask, // Pointer to task code
		  NULL,      // Pointer to argument that is
		  // passed to task
		  (void *)&StartTask_Stack[TASK_STACKSIZE-1], // Pointer to top
		  // of task stack 
		  STARTTASK_PRIO,
		  STARTTASK_PRIO,
		  (void *)&StartTask_Stack[0],
		  TASK_STACKSIZE,
		  (void *) 0,  
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
         
  OSStart();
  
  return 0;
}
