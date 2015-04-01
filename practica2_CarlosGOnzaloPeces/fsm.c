#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "fsm.h"

//Functions and varaibles to measure time

struct timespec start, stop, res, max_time;
// res = a - b
void
timespec_sub (struct timespec *res, struct timespec *a, struct timespec *b)
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (res->tv_nsec < 0) {
		--res->tv_sec;
		res->tv_nsec += 10000000000;
	}
}

int timespec_less (struct timespec *a, struct timespec *b)
{
	return (a->tv_sec < b->tv_sec) ||
	((a->tv_sec == b->tv_sec) && (a->tv_nsec < b->tv_nsec));
}

void register_time (fsm_t* this, struct timespec* new_time)
{

	if (timespec_less (&(this->max_time), new_time)) {
		this->max_time.tv_sec = new_time->tv_sec;
		this->max_time.tv_nsec = new_time->tv_nsec;
	}
}


//Functions of the finite state machine

fsm_t* fsm_new (fsm_trans_t* tt) 
{
	fsm_t* this = (fsm_t*) malloc (sizeof (fsm_t)); 
	fsm_init (this, tt); 
	return this;
}

void fsm_init (fsm_t* this, fsm_trans_t* tt) 
{
	this->tt = tt;
	this->max_time.tv_sec = 0;
	this->max_time.tv_nsec = 0;
}

void fsm_fire (fsm_t* this)
{
	TIME(clock_gettime( CLOCK_MONOTONIC, &start));
	fsm_trans_t* t;
	for (t = this->tt; t->orig_state >= 0; ++t) {
		if ((this->current_state == t->orig_state) && t->in(this)) {
			this->current_state = t->dest_state; 
			if (t->out)
				t->out(this);
			break;
		}
	}	
	
	TIME({clock_gettime( CLOCK_MONOTONIC, &stop);
	timespec_sub(&res, &stop, &start );
	register_time(this,&res);});
	
}




