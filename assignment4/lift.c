#include "lift.h"

/* Simple_OS include */ 
#include <pthread.h>

/* drawing module */ 
#include "draw.h"

/* standard includes */ 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* panic function, to be called when fatal errors occur */ 
static void lift_panic(const char message[])
{
    printf("LIFT_PANIC!!! "); 
    printf("%s", message); 
    printf("\n"); 
    exit(0); 
}

/* --- monitor data type for lift and operations for create and delete START --- */

/* lift_create: creates and initialises a variable of type lift_type */
lift_type lift_create(void) 
{
    /* the lift to be initialised */
    lift_type lift;

    /* floor counter */ 
    int floor; 

    /* loop counter */
    int i;

    /* allocate memory */
    lift = (lift_type) malloc(sizeof(lift_data_type));

    /* initialise variables */

    /* initialise floor */
    lift->floor = 0; 
    
    /* set direction of lift travel to up */
    lift->up = 1;

    /* the lift is not moving */ 
    lift->moving = 0; 

    /* initialise person information */
    for (floor = 0; floor < N_FLOORS; floor++)
    {
        for (i = 0; i < MAX_N_PERSONS; i++)
        {
            lift->persons_to_enter[floor][i].id = NO_ID; 
            lift->persons_to_enter[floor][i].to_floor = NO_FLOOR; 
        }
    }

    /* initialise passenger information */
    for (i = 0; i < MAX_N_PASSENGERS; i++) 
    {
        lift->passengers_in_lift[i].id = NO_ID; 
        lift->passengers_in_lift[i].to_floor = NO_FLOOR; 
    }

    return lift;
}

/* lift_delete: deallocates memory for lift */
void lift_delete(lift_type lift)
{
    free(lift);
}


/* --- monitor data type for lift and operations for create and delete END --- */


/* --- functions related to lift task START --- */

/* MONITOR function lift_next_floor: computes the floor to which the lift 
   shall travel. The parameter *change_direction indicates if the direction 
   shall be changed */
void lift_next_floor(lift_type lift, int *next_floor, int *change_direction)
{
    if (lift->floor == 0 ){
	*change_direction = 1;
	*next_floor = lift->floor + 1;
    }
    else if (lift->floor == N_FLOORS-1){
	*change_direction = 1;
	*next_floor = lift->floor - 1;
	}
    else{
	*change_direction = 0; 
	if(lift->up == 1){
		*next_floor = lift->floor - 1;	
	}else{
		*next_floor = lift->floor + 1;	
	}
    }
}

void lift_move(lift_type lift, int next_floor, int change_direction)
{ 

    /* the lift is moving */ 
    lift->moving = 1;  
        
    /* it takes two seconds to move to the next floor */ 
    usleep(2000000);
        
    /* the lift is not moving */ 
    lift->moving = 0; 

    /* the lift has arrived at next_floor */ 
    lift->floor = next_floor; 

    /* check if direction shall be changed */ 
    if (change_direction)
    {
        lift->up = !lift->up; 
    } 
	
	//printf("CURRENT FLOOR IN LIFT MOVE: %d\n",lift->floor);
	//printf("lift moving in lift move: %d\n",lift->moving);
    /* release lift */ 
}

/* this function is used also by the person tasks */ 
int n_passengers_in_lift(lift_type lift)
{
    int n_passengers = 0; 
    int i; 
        
    for (i = 0; i < MAX_N_PASSENGERS; i++)
    {
        if (lift->passengers_in_lift[i].id != NO_ID)
        {
            n_passengers++; 
        }
    }
    return n_passengers; 
}

int persons_to_enter_lift(lift_type lift)
{
	int i;
	int n_passengers = 0;

	for(i = 0; i < MAX_N_PERSONS; i++){
		if(lift->persons_to_enter[lift->floor][i].id != NO_ID 
		   && lift->persons_to_enter[lift->floor][i].to_floor != NO_FLOOR){
			n_passengers++;
		}
	}
	
	return n_passengers;
}

int persons_to_exit_lift(lift_type lift)
{
	int i;
	int n_passengers = 0;
	for (i = 0; i < MAX_N_PASSENGERS; i++)
        {
        	if (lift->passengers_in_lift[i].id != NO_ID && lift->passengers_in_lift[i].to_floor == lift->floor)
        	{
            		n_passengers++; 
        	}
        }
        return n_passengers;
}


/* MONITOR function lift_has_arrived: shall be called by the lift task
   when the lift has arrived at the next floor. This function indicates
   to other tasks that the lift has arrived, and then waits until the lift
   shall move again. */
/*void lift_has_arrived(lift_type lift)
{
	//Väntar tills alla som ska av har gått av
	while(persons_to_exit_lift(lift) != 0){
		pthread_cond_wait(&lift->change, &lift->mutex);
	}	
	
	//Väntar tills alla som ska på och som kan gå på har gått på
	while(persons_to_enter_lift(lift) != 0 && n_passengers_in_lift(lift) < MAX_N_PASSENGERS){
		pthread_cond_wait(&lift->change, &lift->mutex);		
	}	
}*/


/* --- functions related to lift task END --- */


/* --- functions related to person task START --- */

static void person_init(lift_type lift, int id, int from_floor, int to_floor)
{
    int i;
    int found;
    
    found = 0;
    for(i = 0; i < MAX_N_PERSONS && !found; i++){
	if (lift->persons_to_enter[from_floor][i].id == NO_ID){
		found = 1;		
		lift->persons_to_enter[from_floor][i].id = id;
		lift->persons_to_enter[from_floor][i].to_floor = to_floor;
	}
    }
    if (!found){
	lift_panic("cannot initiate person");
    }
}
/* passenger_wait_for_lift: returns non-zero if the passenger shall
   wait for the lift, otherwise returns zero */
static int passenger_wait_for_lift(lift_type lift, int wait_floor)
{

    int waiting_ready =
        /* the lift is not moving */ 
        !lift->moving && 
        /* and the lift is at wait_floor */ 
        lift->floor == wait_floor && 
        /* and the lift is not full */ 
        n_passengers_in_lift(lift) < MAX_N_PASSENGERS; 
	
    return !waiting_ready;
}

void enter_lift(lift_type lift, int id, int to_floor)
{
    int i;
    int found;
    
    found = 0;
    for(i = 0; i < MAX_N_PASSENGERS && !found; i++){
	if (lift->passengers_in_lift[i].id == NO_ID){
		found = 1;		
		lift->passengers_in_lift[i].id = id;
		lift->passengers_in_lift[i].to_floor = to_floor;
	}
    }
    if (!found){
	lift_panic("cannot enter lift");
    }
}

void exit_lift(lift_type lift, int *id)
{
    int i;
    int found;
    
    found = 0;
    for(i = 0; i < MAX_N_PASSENGERS && !found; i++){
	if (lift->passengers_in_lift[i].to_floor == lift->floor){
		found = 1;	
		*id = lift->passengers_in_lift[i].id; 	
		lift->passengers_in_lift[i].id = NO_ID;
		lift->passengers_in_lift[i].to_floor = NO_FLOOR;
		
	}
    }
    if (!found){
	lift_panic("cannot exit lift");
    }
}

/* enter_floor: makes a person with id id stand at floor floor */ 
void enter_floor(
		 lift_type lift, int id, int floor, int to_floor)
{
    int i; 
    int floor_index; 
    int found; 

    /* stand at floor */ 
    found = 0; 
    for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
        if (lift->persons_to_enter[floor][i].id == NO_ID)
        {
            found = 1; 
            floor_index = i; 
        }
    }
        
    if (!found)
    {
        lift_panic("cannot enter floor"); 
    }

    /* enter floor at index floor_index */ 
    lift->persons_to_enter[floor][floor_index].id = id; 
    lift->persons_to_enter[floor][floor_index].to_floor = to_floor;
}

/* leave_floor: makes a person with id id at enter_floor leave 
   enter_floor */ 
void leave_floor(
    lift_type lift, int enter_floor, int *id, int *to_floor)

/* fig_end lift_c_prot */ 
{
    int i; 
    int floor_index; 
    int found; 

    /* leave the floor */
    found = 0; 
    for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
        if (lift->persons_to_enter[enter_floor][i].to_floor != NO_FLOOR)
        {
            found = 1; 
            floor_index = i;
	    *id = lift->persons_to_enter[enter_floor][i].id;
	    *to_floor = lift->persons_to_enter[enter_floor][i].to_floor; 
        }
    }
        
    if (!found)
    {
        lift_panic("cannot leave floor"); 
    }

    /* leave floor at index floor_index */ 
    lift->persons_to_enter[enter_floor][floor_index].id = NO_ID; 
    lift->persons_to_enter[enter_floor][floor_index].to_floor = NO_FLOOR;
}

/* --- functions related to person task END --- */
