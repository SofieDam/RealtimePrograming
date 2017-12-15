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
    //lift->moving = 0;

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
    if(((lift->floor) < (N_FLOORS - 1)) && ((lift->up)==1))
    {
      *change_direction = 0;
      *next_floor = lift->floor + 1;
    }
    else if (((lift->floor) > 0) && ((lift->up)==0))
    {
      *change_direction = 0;
      *next_floor = lift->floor - 1;
    }
    else if ((lift->floor) == 0)
    {
      *change_direction = 1;
    }
    else if ((lift->floor) == (N_FLOORS - 1))
    {
      *change_direction = 1;
    }
    }

/* MONITOR function lift_move: makes the lift move from its current
   floor to next_floor. The parameter change_direction indicates if
   the move includes a change of direction. This function shall be
   called by the lift process when the lift shall move */
void lift_move(lift_type lift, int next_floor, int change_direction)
{
    /* reserve lift */
  //pthread_mutex_lock(&lift->mutex);

    /* the lift is moving */
    //lift->moving = 1;

    /* release lift */
    //pthread_mutex_unlock(&lift->mutex);

    /* it takes two seconds to move to the next floor */
    //usleep(2000000);

    /* reserve lift */
    //pthread_mutex_lock(&lift->mutex);

    /* the lift is not moving */
    //lift->moving = 0;

    /* the lift has arrived at next_floor */
    lift->floor = next_floor;

    /* check if direction shall be changed */
    if (change_direction)
    {
        lift->up = !lift->up;
    }

    /* draw, since a change has occurred */
    //draw_lift(lift);

    /* release lift */
    //pthread_mutex_unlock(&lift->mutex);
}

/* this function is used also by the person tasks */
static int n_passengers_in_lift(lift_type lift)
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

/* MONITOR function lift_has_arrived: shall be called by the lift task
   when the lift has arrived at the next floor. This function indicates
   to other tasks that the lift has arrived, and then waits until the lift
   shall move again. */
void lift_has_arrived(lift_type lift)
{
  
    usleep(2000000);
  /*if()
  {

  }*/
}

/* --- functions related to lift task END --- */


/* --- functions related to person task START --- */

/* passenger_wait_for_lift: returns non-zero if the passenger shall
   wait for the lift, otherwise returns zero */
static int passenger_wait_for_lift(lift_type lift, int wait_floor)
{
    int waiting_ready =
        /* the lift is not moving */
        //!lift->moving &&
        /* and the lift is at wait_floor */
        lift->floor == wait_floor &&
        /* and the lift is not full */
        n_passengers_in_lift(lift) < MAX_N_PASSENGERS;

    return !waiting_ready;
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
    lift_type lift, int floor, int *id, int *to_floor)

/* fig_end lift_c_prot */
{
    int i;
    int floor_index;
    int found;

    /* leave the floor */
    found = 0;
    for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
        if (lift->persons_to_enter[floor][i].id == *id)
        {
            found = 1;
            floor_index = i;
        }
    }

    if (!found)
    {
        lift_panic("cannot leave floor");
    }

    /* leave floor at index floor_index */
    lift->persons_to_enter[floor][floor_index].id = NO_ID;
    lift->persons_to_enter[floor][floor_index].to_floor = NO_FLOOR;
}

// Passenger enter lift
void enter_lift(lift_type lift, int id, int to_floor)
{
  int i;
  for(i = 0; i < MAX_N_PASSENGERS; i++)
  {
    if(lift->passengers_in_lift[i].id == NO_ID)
    {
      lift->passengers_in_lift[i].id = i;
      lift->passengers_in_lift[i].to_floor = to_floor;
      break;
    }

  }
}

// Passenger leave lift
void leave_lift(lift_type lift, int floor,  int *id)
{
     printf("in leave_lift, to floor %d\n",lift->passengers_in_lift[i].to_floor);
  int i;
  for(i = 0; i < MAX_N_PASSENGERS; i++)
  {
  
    if(lift->passengers_in_lift[i].to_floor == floor && lift->passengers_in_lift[i].id == *id)
    {
     
      lift->passengers_in_lift[i].id = NO_ID;
      lift->passengers_in_lift[i].to_floor = NO_FLOOR;
      break;
    }
  }
}

/* n_persons_to_enter: returns the number of persons standing on
   floor floor */
int n_persons_to_enter(lift_type lift, int floor)
{
  int i;
  int number_of_persons = 0;
  for(i = 0; i < MAX_N_PERSONS; i++)
    {
      if(lift->persons_to_enter[floor][i].id != NO_ID)
	{
	  ++number_of_persons;
	}
    }
  return number_of_persons;
}

/* MONITOR function lift_travel: makes the person with id id perform
   a journey with the lift, starting at from_floor and ending
   at to_floor */
/*
void lift_travel(lift_type lift, int id, int from_floor, int to_floor)
{
    enter_floor(lift, id, from_floor);
    // true (1) when passenger shall wait
    while (passenger_wait_for_lift(lift, from_floor))
    {
        pthread_cond_wait(&lift->change, &lift->mutex);
    }

    leave_floor(lift, id, from_floor);

    enter_lift(lift, id, to_floor);

    while (to_floor != lift->floor)
    {
        pthread_cond_wait(&lift->change, &lift->mutex);
    }
    leave_lift(lift, id);

}
*/

 /*
int lift_is_full(lift_type lift)
{
  int number_of_persons;
  for(number_of_persons = 0; number_of_persons < MAX_N_PASSENGERS; number_of_persons++)
  {
     return 0;
  }
}

 */
/* --- functions related to person task END --- */
