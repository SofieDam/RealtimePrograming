/* standard library includes */ 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "display.h"
#include "si_ui.h"


/* time data type */ 
typedef struct 
{
    int hours; 
    int minutes; 
    int seconds; 
} time_data_type; 


/* clock data type */ 
typedef struct
{
    /* the current time */ 
    time_data_type time; 
    /* alarm time */ 
    time_data_type alarm_time; 
    /* alarm enabled flag */ 
    int alarm_enabled; 

    /* semaphore for mutual exclusion */ 
    pthread_mutex_t mutex; 

    /* semaphore for alarm activation */ 
    sem_t start_alarm; 

} clock_data_type; 


/* the actual clock */ 
static clock_data_type Clock; 


/* clock_init: initialise clock */ 
void clock_init(void)
{
    /* initialise time and alarm time */ 

    Clock.time.hours = 0; 
    Clock.time.minutes = 0; 
    Clock.time.seconds = 0; 

    Clock.alarm_time.hours = 0; 
    Clock.alarm_time.minutes = 0; 
    Clock.alarm_time.seconds = 0; 
    
    /* alarm is not enabled */ 
    Clock.alarm_enabled = 0; 

    /* initialise semaphores */ 
    pthread_mutex_init(&Clock.mutex, NULL); 
    sem_init(&Clock.start_alarm, 0, 0); 
}



/* increment_time: increments the current time with 
   one second */ 
void increment_time(void)
{
    /* reserve clock variables */ 
    pthread_mutex_lock(&Clock.mutex);

    /* increment time */ 
    Clock.time.seconds++; 
    if (Clock.time.seconds > 59)
    {
        Clock.time.seconds = 0; 
        Clock.time.minutes++; 
        if (Clock.time.minutes > 59)
        {
            Clock.time.minutes = 0; 
            Clock.time.hours++; 
            if (Clock.time.hours > 12)
            {
                Clock.time.hours = 1; 
            }
        }
    }

    /* release clock variables */ 
    pthread_mutex_unlock(&Clock.mutex);
}


/* clock_set_time: set current time to hours, minutes and seconds */ 
void clock_set_time(int hours, int minutes, int seconds)
{
    pthread_mutex_lock(&Clock.mutex); 

    Clock.time.hours = hours; 
    Clock.time.minutes = minutes; 
    Clock.time.seconds = seconds; 

    pthread_mutex_unlock(&Clock.mutex); 
}

//-------------------------------------------------------------------

/* clock_set_time: set current time to hours, minutes and seconds */ 
void clock_set_alarm_time(int hours, int minutes, int seconds)
{
    pthread_mutex_lock(&Clock.mutex); 

    Clock.alarm_time.hours = hours; 
    Clock.alarm_time.minutes = minutes; 
    Clock.alarm_time.seconds = seconds; 

    pthread_mutex_unlock(&Clock.mutex); 
}
//-------------------------------------------------------------------

/* get_time: read time from common clock variables */
void get_time(int *hours, int *minutes, int *seconds)
{
    /* reserve clock variables */
    pthread_mutex_lock(&Clock.mutex);

    /* read values */
    *hours = Clock.time.hours;
    *minutes = Clock.time.minutes;
    *seconds = Clock.time.seconds;
        
    /* release clock variables */
    pthread_mutex_unlock(&Clock.mutex);
}


/* time_from_set_time_message: extract time from set time 
   message from user interface */ 
void time_from_set_time_message(
    char message[], int *hours, int *minutes, int *seconds)
{
    sscanf(message,"set %d %d %d", hours, minutes, seconds); 
}

/* time_from_set_alarm_time_message: extract time from set alarm_time 
   message from user interface */ 
void time_from_set_alarm_time_message(
    char message[], int *hours, int *minutes, int *seconds)
{
    sscanf(message,"alarm %d %d %d", hours, minutes, seconds); 
}



/* time_ok: returns nonzero if hours, minutes and seconds represents a valid time */ 
int time_ok(int hours, int minutes, int seconds)
{
    return hours >= 0 && hours <= 24 && minutes >= 0 && minutes <= 59 && 
        seconds >= 0 && seconds <= 59; 
}





/* clock_task: clock task */ 
void *clock_thread(void *unused) 
{
    /* local copies of the current time */ 
    int hours, minutes, seconds; 

    /* infinite loop */ 
    while (1)
    {
      //sem_post(&Clock.start_alarm);
        /* read and display current time */ 
	 if(Clock.time.hours == Clock.alarm_time.hours && 
	 Clock.time.minutes == Clock.alarm_time.minutes &&
	 Clock.time.seconds == Clock.alarm_time.seconds )
	   {
	     if(Clock.alarm_enabled == 1)
	       {
		 sem_post(&Clock.start_alarm);
	       }
	   }
        get_time(&hours, &minutes, &seconds); 
        display_time(hours, minutes, seconds); 

        /* increment time */ 
        increment_time(); 

        /* wait one second */ 
        usleep(1000000);
    }
}


//-----------------------------------------------------------------
/* clock_task: clock task */ 
void *repeat_alarm_thread(void *unused) 
{
    /* local copies of the current time */ 
    //int hours, minutes, seconds; 

    /* infinite loop */ 

    while (1)
    {
      sem_wait(&Clock.start_alarm);
      /* wait 1,5 second */ 
      while(Clock.alarm_enabled == 1)
	{
	  display_alarm_text();
	  usleep(1500000);
	}
	
    }
}
//-----------------------------------------------------------------

/* set_task: reads messages from the user interface, and 
   sets the clock, or exits the program */ 
void * set_thread(void *unused)
{
    /* message array */ 
    char message[SI_UI_MAX_MESSAGE_SIZE]; 

    /* time read from user interface */ 
    int hours, minutes, seconds; 

    /* set GUI size */ 
    si_ui_set_size(400, 200); 

    while(1)
    {
        /* read a message */ 
        si_ui_receive(message); 
        /* check if it is a set message */ 
        if (strncmp(message, "set", 3) == 0)
        {
            time_from_set_time_message(message, &hours, &minutes, &seconds); 
            if (time_ok(hours, minutes, seconds))
            {
                clock_set_time(hours, minutes, seconds); 
            }
	    else
            {
                si_ui_show_error("Illegal value for hours, minutes or seconds"); 
            } 
	}
	/* check if it is a set alarm time message */ 
	else if (strncmp(message, "alarm", 5) == 0)
	{
	  time_from_set_alarm_time_message(message, &hours, &minutes, &seconds); 
            if (time_ok(hours, minutes, seconds))
            {
             
	      Clock.alarm_enabled = 1;
	      clock_set_alarm_time(hours, minutes, seconds); 
	      display_alarm_time(hours, minutes, seconds);
	    
            }
            else
            {
                si_ui_show_error("Illegal value for hours, minutes or seconds"); 
            }
        }
//--------------------------------------------------------------
	/* check if it is an reset message */ 
        else if (strcmp(message, "reset") == 0)
        {
	 
	  Clock.alarm_enabled = 0;
	  erase_alarm_time();
	  erase_alarm_text();

        }

//---------------------------------------------------------------
        /* check if it is an exit message */ 
        else if (strcmp(message, "exit") == 0)
        {
            exit(0); 
        }
        /* not a legal message */ 
        else
        {
            si_ui_show_error("unexpected message type"); 
        }
    }
}



/* main */ 
int main(void)
{
    /* initialise UI channel */ 
    si_ui_init(); 

    /* initialise variables */         
    clock_init(); 
    /* initialise variables */        
    display_init();

    /* create tasks */ 
    pthread_t clock_thread_handle;
    pthread_t set_thread_handle;
    pthread_t repeat_alarm_thread_handle;

    pthread_create(&clock_thread_handle, NULL, clock_thread, 0);
    pthread_create(&set_thread_handle, NULL, set_thread, 0);
    pthread_create(&repeat_alarm_thread_handle, NULL, repeat_alarm_thread, 0);

    pthread_join(clock_thread_handle, NULL);
    pthread_join(set_thread_handle, NULL);
    pthread_join(repeat_alarm_thread_handle, NULL);
    /* will never be here! */ 
    return 0; 
}

