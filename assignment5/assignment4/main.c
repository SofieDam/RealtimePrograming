#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include "lift.h"
#include "si_ui.h"
#include "messages.h"

#include "draw.h"

#define QUEUE_UI 0
#define QUEUE_LIFT 1
#define QUEUE_FIRSTPERSON 10

// These variables keeps track of the process IDs of all processes
// involved in the application so that they can be killed when the
// exit command is received.
static pid_t lift_pid;
static pid_t uidraw_pid;
static pid_t liftmove_pid;
static pid_t person_pid[MAX_N_PERSONS];
static pid_t uicommand_pid;

typedef enum {LIFT_TRAVEL, // A travel message is sent to the list process when a person would
	                   // like to make a lift travel
	      LIFT_TRAVEL_DONE, // A travel done message is sent to a person process when a
	                        // lift travel is finished
	      LIFT_MOVE         // A move message is sent to the lift task when the lift shall move
	                        // to the next floor
} lift_msg_type;

struct lift_msg{
	lift_msg_type type;  // Type of message
	int person_id;       // Specifies the person
	int from_floor;      // Specify source and destion for the LIFT_TRAVEL message.
	int to_floor;
};



// Since we use processes now and not
static int get_random_value(int person_id, int maximum_value)
{
	return rand() % (maximum_value + 1);
}


// Initialize the random seeds used by the get_random_value() function
// above.
static void init_random(void)
{
	srand(getpid()); // The pid should be a good enough initialization for
                         // this case at least.
}


static void liftmove_process(void)
{
	struct lift_msg m;
	m.type = LIFT_MOVE;
	while(1){
		// TODO:
		//    Sleep 2 seconds
		usleep(2000000);
                //    Send a message to the lift process to move the lift.
		message_send((char *) &m,sizeof m, QUEUE_LIFT, 0);
	}
}


static void lift_process(void)
{
        lift_type Lift;
	Lift = lift_create();
	int change_direction, next_floor;

	char msgbuf[4096];
	while(1){
		int i;
		struct lift_msg reply;
		struct lift_msg *m;
		message_send((char *) Lift, sizeof(*Lift), QUEUE_UI,0); // Draw the lift
		int len = message_receive(msgbuf, 4096, QUEUE_LIFT); // Wait for a message
		if(len < sizeof(struct lift_msg)){
			fprintf(stderr, "Message too short\n");
			continue;
		}

		m = (struct lift_msg *) msgbuf;
		switch(m->type){
		case LIFT_MOVE:
			// TODO:
		  reply.type = LIFT_TRAVEL_DONE;
			//    Check if passengers want to leave elevator
			while(persons_to_exit_lift(Lift) != 0){
                        //        Remove the passenger from the elevator
			  exit_lift(Lift, &reply.person_id);
                        //        Send a LIFT_TRAVEL_DONE for each passenger that leaves
                        //        the elevator
			  message_send((char *) &reply, sizeof reply, QUEUE_FIRSTPERSON+reply.person_id, 0);
			  printf("sent message: %d\n", reply.person_id);
			  //enter_floor(Lift, reply.person_id, Lift->floor, reply.to_floor);
			  //leave_floor(Lift, Lift->floor, &reply.person_id, &reply.to_floor);
			}
			//    Check if passengers want to enter elevator
			while(persons_to_enter_lift(Lift) != 0 && n_passengers_in_lift(Lift) < MAX_N_PASSENGERS){
			  // Remove the passenger from the floor and into the elevator
			  leave_floor(Lift, Lift->floor, &m->person_id, &m->to_floor);
			  enter_lift(Lift, m->person_id, m->to_floor);
			}
			//    Move the lift

			lift_next_floor(Lift, &next_floor, &change_direction);
			lift_move(Lift, next_floor, change_direction);
			break;
		case LIFT_TRAVEL:
                        // TODO:
                        //    Update the Lift structure so that the person with the given ID is now present on the floor
		  enter_floor(Lift, m->person_id, m->from_floor, m->to_floor);
			break;
		}
	}
	return;
}

static void person_process(int id)
{
	init_random();
	int to_floor, from_floor;
	char buf[4096];
	struct lift_msg m;
	struct lift_msg *receive;
	m.type = LIFT_TRAVEL;

	while(1){
		// TODO:
		//    Generate a to and from floor
		m.from_floor = get_random_value(id, N_FLOORS-1);
		m.to_floor = get_random_value(id, N_FLOORS-1);
		m.person_id = id;

		while(m.from_floor == m.to_floor)
		{
			m.to_floor = get_random_value(id, 4);
		}

		//    Send a LIFT_TRAVEL message to the lift process
		message_send((char *) &m, sizeof m, QUEUE_LIFT, 0);
                //    Wait for a LIFT_TRAVEL_DONE message
		int len = message_receive(buf, 4096, QUEUE_FIRSTPERSON+id);

		if(len < sizeof(struct lift_msg)){
			fprintf(stderr, "Message too short\n");
			continue;
		}
		printf("recived message\n");
		receive = (struct lift_msg *) buf;
		while(receive->type != LIFT_TRAVEL_DONE){
			len = message_receive(buf, 4096, QUEUE_FIRSTPERSON+id);
			receive = (struct lift_msg *) buf;
		}
		printf("travel done\n");
		//    Wait a little while
		usleep(5000000);
	}
}

// This is the final process called by main()
// It is responsible for:
//   * Receiving and executing commands from the java GUI
//   * Killing off all processes when exiting the application
void uicommand_process(void)
{
	int i;
	int current_person_id = 0;
	char message[SI_UI_MAX_MESSAGE_SIZE];
	while(1){
		// Read a message from the GUI
		si_ui_receive(message);
		if(!strcmp(message, "new"))
		{
			// TODO:
			// * Check that we don't create too many persons
			//printf("create new person\n");
			// * fork and create a new person process (and
			//   record the new pid in person_pid[])
			if (current_person_id < MAX_N_PERSONS)
			{
				person_pid[current_person_id] = fork();
				printf("person_id:%d\n", person_pid[current_person_id]);
				if (!person_pid[current_person_id])
				{
					person_process(current_person_id);
				}
				current_person_id++;
			}
		}

		else if(!strcmp(message, "exit")){
			// The code below sends the SIGINT signal to
			// all processes involved in this application
			// except for the uicommand process itself
			// (which is exited by calling exit())
			kill(uidraw_pid, SIGINT);
			kill(lift_pid, SIGINT);
			kill(liftmove_pid, SIGINT);
			for(i=0; i < MAX_N_PERSONS; i++)
			{
				if(person_pid[i] > 0)
				{
					kill(person_pid[i], SIGINT);
				}
			}
			exit(0);
		}
	}
}

// This process is responsible for drawing the lift. Receives lift_type structures
// as messages.
void uidraw_process(void)
{
	char msg[1024];
	si_ui_set_size(670, 700);
	while(1)
	{
			message_receive(msg, 1024, QUEUE_UI);
			lift_type Lift = (lift_type) &msg[0];
			draw_lift(Lift);
	}
}

/* ------------------- MAIN -------------------------------------*/
int main(int argc, char **argv)
{
	message_init();
        si_ui_init(); // Initialize user interface. (Must be done
		      // here!)

	lift_pid = fork();
	if(!lift_pid) {
		lift_process();
	}
	uidraw_pid = fork();
	if(!uidraw_pid){
		uidraw_process();
	}
	liftmove_pid = fork();
	if(!liftmove_pid){
		liftmove_process();
	}

	uicommand_pid = fork();
	if(!uicommand_pid)
	{
		uicommand_process();
	}

	/*int i;
	for (i = 0; i < MAX_N_PERSONS; i++){
		// * fork and create a new person process (and
		//   record the new pid in person_pid[])
		person_pid[i] = fork();
		if (!person_pid[i]){
		person_process(i);
		}
	}
	uicommand_process();
*/
	return 0;
}
