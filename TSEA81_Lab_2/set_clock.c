/* standard library includes */ 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

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
    time_data_type current_time; 
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

/* get_time: read time from common clock variables */ 
void get_time(int *hours, int *minutes, int *seconds)
{
    /* reserve clock variables */ 
    pthread_mutex_lock(&Clock.mutex);

    /* read values */ 
    *Clock.time.hours = hours; 
    *Clock.time.minutes = minutes; 
    *Clock.time.seconds = seconds;
        
    /* release clock variables */ 
    pthread_mutex_unlock(&Clock.mutex);
}
