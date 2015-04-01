
#ifndef FSM_H

#define FSM_H

#define MAXSTATES 10

#define MAXINS 32
#define NDEBUG
//#define TIMEFIRE
#define TIMERESOURCE

#ifndef NDEBUG
	#define DEBUG(x) x
#else
	#define DEBUG(x)
#endif

#ifdef TIMEFIRE
	#define TIME(x) x
#else 
	#define TIME(x)
#endif

#ifdef TIMERESOURCE
	#define TIME2(x) x
#else 
	#define TIME2(x)
#endif



typedef struct fsm_t fsm_t;

typedef int (*fsm_input_func_t) (fsm_t*); 

typedef void (*fsm_output_func_t) (fsm_t*); 

typedef struct fsm_trans_t {

	int orig_state;
	fsm_input_func_t in;
	int dest_state;
	fsm_output_func_t out;


	} fsm_trans_t;

struct fsm_t {

	int current_state;

	fsm_trans_t* tt;

	struct timespec max_time;
};

fsm_t* fsm_new (fsm_trans_t* tt); 

void fsm_init (fsm_t* this, fsm_trans_t* tt); 

void fsm_fire (fsm_t* this); 

void timespec_sub (struct timespec *res, struct timespec *a, struct timespec *b);

int timespec_less (struct timespec *a, struct timespec *b);

#endif