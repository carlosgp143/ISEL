#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <wiringPi.h>
#include <stdio.h>
#include "fsm.h"
#include "task.h"
#define GPIO_BUTTON	2
#define GPIO_LED	3
#define GPIO_CUP	4
#define GPIO_COFFEE	5
#define GPIO_MILK	6
#define GPIO_CHANGE	7
#define GPIO_COIN	8
#define GPIO_BUTTON_CHANGE	9
#define CUP_TIME	300
#define COFFEE_TIME	3000
#define MILK_TIME	3000

struct timespec start1, stop1, total1, max1;
struct timespec start2, stop2, total2, max2;
struct timespec start3, stop3, total3, max3;

//register the time to access to a share resource
void register_time_resource (struct timespec* max, struct timespec* new_time)
{

	if (timespec_less (max, new_time)) {
		max->tv_sec = new_time->tv_sec;
		max->tv_nsec = new_time->tv_nsec;
	}
}



enum cofm_state {//estados de la maquina de cafe
	COFM_WAITING,
	COFM_CUP,
	COFM_COFFEE,
	COFM_MILK,
};

enum mon_state{//estados del monedero
	WAITING_MONEY,
	WAITING_FINISH,

};
fsm_t* cofm_fsm;
fsm_t* wallet_fsm;

pthread_mutex_t m_scr;


//variables de control del monedero
static int money = 0;
static int coin_in = 0;
static int coin = 0;
static int can_give_coffe = 0;
static int coffe_given = 0;
static int change = 0;
static int button_change = 0;
static int giving_coffe = 0;

int readCoin(){
	int ret=0;
	DEBUG(printf("Introduzca moneda: "););
	scanf("%d", &ret);
	DEBUG(printf("\n"););
	return ret;
}

static void coin_isr (void) { coin = 1 ;}
static void button_change_isr (void) { button_change = 1 ;}



static int button = 0;

static void button_isr (void) { button = 1; }

static int timer = 0;

static void timer_isr (union sigval arg) { timer = 1; }

static void timer_start (int ms)
{
	timer_t timerid;
	struct itimerspec value;
	struct sigevent se;
	se.sigev_notify = SIGEV_THREAD;
	se.sigev_value.sival_ptr = &timerid;
	se.sigev_notify_function = timer_isr;
	se.sigev_notify_attributes = NULL;
	value.it_value.tv_sec = ms / 1000;
	value.it_value.tv_nsec = (ms % 1000) * 1000000;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = 0;
	timer_create (CLOCK_REALTIME, &se, &timerid);
	timer_settime (timerid, 0, &value, NULL);
}

static int button_pressed (fsm_t* this)
{
	int ret = button; 
	button = 0;
	int ret2 = 0;

	pthread_mutex_lock(&m_scr);
	if(can_give_coffe){
		ret2 = 1;
	}
	pthread_mutex_unlock(&m_scr);

	return ret&&ret2;
}

static int timer_finished (fsm_t* this){

	int ret = timer;
	timer = 0;
	return ret;
}


static int button_change_pressed (fsm_t* this)
{
	int ret = button_change; 
	button_change = 0;
	int ret2 = 0;
	pthread_mutex_lock(&m_scr);
	if(!giving_coffe){
		ret2 = 1;
	}
	pthread_mutex_unlock(&m_scr);
	return ret&&ret2;
	
}


static int money_in (fsm_t* this){
	if(coin){
		coin_in = readCoin();

		money += coin_in; 
		DEBUG(printf("El dinero almacenado es %d\n", money));
		coin=0;
		if(money >= 50){
			return 1;
			}
		else {
			return 0;
		}
		

	}
	else{
		return 0;
	}

	
}

static int coffe_finished (fsm_t* this){
	TIME2(clock_gettime( CLOCK_MONOTONIC, &start2));

	

	int ret = 0;

	pthread_mutex_lock(&m_scr);
	if (coffe_given){
		ret = 1;
	}
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop2);
	timespec_sub(&total2, &stop2, &start2);
	register_time_resource(&max2,&total2);});

	return ret;

}


static void cup (fsm_t* this)
{
	DEBUG(printf("Saco la taza\n"));

	TIME2(clock_gettime( CLOCK_MONOTONIC, &start3));

	pthread_mutex_lock(&m_scr);
	giving_coffe = 1;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop3);
	timespec_sub(&total3, &stop3, &start3);
	register_time_resource(&max3,&total3);});

	digitalWrite (GPIO_LED, LOW);
	digitalWrite (GPIO_CUP, HIGH);
	timer_start (CUP_TIME);
}

static void coffee (fsm_t* this)
{
	DEBUG(printf("Saco el café\n"));
	digitalWrite (GPIO_CUP, LOW);
	digitalWrite (GPIO_COFFEE, HIGH);
	timer_start (COFFEE_TIME);
}

static void milk (fsm_t* this)
{
	DEBUG(printf("Saco la leche\n"));
	digitalWrite (GPIO_COFFEE, LOW);
	digitalWrite (GPIO_MILK, HIGH);
	timer_start (MILK_TIME);
}

static void finish (fsm_t* this)
{
	DEBUG(printf("Acabo el proceso de dispensado del cafe\n"));
	digitalWrite (GPIO_MILK, LOW);
	digitalWrite (GPIO_LED, HIGH);
	TIME2(clock_gettime( CLOCK_MONOTONIC, &start2));

	pthread_mutex_lock(&m_scr);
	coffe_given = 1;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop2);
	timespec_sub(&total2, &stop2, &start2);
	register_time_resource(&max2,&total2);});

}

static void money_enough(fsm_t* this){
	DEBUG(printf("Tengo dinero suficiente\n"));

	TIME2(clock_gettime( CLOCK_MONOTONIC, &start1));

	pthread_mutex_lock(&m_scr);
	can_give_coffe = 1;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop1);
	timespec_sub(&total1, &stop1, &start1);
	register_time_resource(&max1,&total1);});
}

static void give_change (fsm_t* this){
	digitalWrite(GPIO_CHANGE, HIGH);
	change = money-50;
	DEBUG(printf("Devuelvo el cambio: %d\n", change););
	
	money = 0;

	TIME2(clock_gettime( CLOCK_MONOTONIC, &start1));

	pthread_mutex_lock(&m_scr);
	can_give_coffe = 0;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop1);
	timespec_sub(&total1, &stop1, &start1);
	register_time_resource(&max1,&total1);});

	coin_in = 0;

	TIME2(clock_gettime( CLOCK_MONOTONIC, &start2));

	pthread_mutex_lock(&m_scr);
	coffe_given = 0;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop2);
	timespec_sub(&total2, &stop2, &start2);
	register_time_resource(&max2,&total2);});


	TIME2(clock_gettime( CLOCK_MONOTONIC, &start3));

	pthread_mutex_lock(&m_scr);
	giving_coffe = 0;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop3);
	timespec_sub(&total3, &stop3, &start3);
	register_time_resource(&max3,&total3);});

}

static void give_money (fsm_t* this){
	
	digitalWrite(GPIO_CHANGE, HIGH);
	change = money;
	DEBUG(printf("Devuelvo el dinero: %d\n", change););

	money = 0;
	TIME2(clock_gettime( CLOCK_MONOTONIC, &start1));

	pthread_mutex_lock(&m_scr);
	can_give_coffe = 0;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop1);
	timespec_sub(&total1, &stop1, &start1);
	register_time_resource(&max1,&total1);});
	coin_in = 0;

	TIME2(clock_gettime( CLOCK_MONOTONIC, &start2));

	pthread_mutex_lock(&m_scr);
	coffe_given = 0;
	pthread_mutex_unlock(&m_scr);

	TIME2({clock_gettime( CLOCK_MONOTONIC, &stop2);
	timespec_sub(&total2, &stop2, &start2);
	register_time_resource(&max2,&total2);});


}

static void do_nothing(fsm_t* this){
	DEBUG(printf("No hago nada\n"));
}
// Explicit FSM description
static fsm_trans_t cofm[] = {
	{ COFM_WAITING, button_pressed, COFM_CUP, cup },
	{ COFM_CUP, timer_finished, COFM_COFFEE, coffee },
	{ COFM_COFFEE, timer_finished, COFM_MILK, milk },
	{ COFM_MILK, timer_finished, COFM_WAITING, finish },
	{-1, NULL, -1, NULL },
};


// Explicit FSM description
static fsm_trans_t wallet[] = {
	{ WAITING_MONEY, money_in, WAITING_FINISH, money_enough},
	{ WAITING_MONEY, button_change_pressed, WAITING_MONEY, give_money },
	{ WAITING_FINISH, coffe_finished, WAITING_MONEY, give_change },
	{ WAITING_FINISH, money_in, WAITING_FINISH, do_nothing },
	{ WAITING_FINISH, button_change_pressed, WAITING_MONEY, give_money },
	{-1, NULL, -1, NULL },
};

// Utility functions, should be elsewhere
// res = a - b
void
timeval_sub (struct timeval *res, struct timeval *a, struct timeval *b)
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_usec = a->tv_usec - b->tv_usec;
	if (res->tv_usec < 0) {
		--res->tv_sec;
		res->tv_usec += 1000000;
	}
}

// res = a + b
void
timeval_add (struct timeval *res, struct timeval *a, struct timeval *b)
{
	res->tv_sec = a->tv_sec + b->tv_sec
	+ a->tv_usec / 1000000 + b->tv_usec / 1000000;
	res->tv_usec = a->tv_usec % 1000000 + b->tv_usec % 1000000;
}

// wait until next_activation (absolute time)
void delay_until (struct timeval* next_activation)
{
	struct timeval now, timeout;
	gettimeofday (&now, NULL);
	timeval_sub (&timeout, next_activation, &now);
	select (0, NULL, NULL, NULL, &timeout);
}

//functions for our tasks

static void *task1_func (void* arg) //cambiar clock_gettime y nanosleep
{
	struct timeval next_activation;
	struct timeval now, timeout;
	gettimeofday (&next_activation, NULL);
	while (1) {
		struct timeval *period = task_get_period (pthread_self());
		timeval_add (&next_activation, &next_activation, period);
		gettimeofday (&now, NULL);
		timeval_sub (&timeout, &next_activation, &now);
		select (0, NULL, NULL, NULL, &timeout) ;
		fsm_fire (cofm_fsm);
	}
}

static void *task2_func (void* arg) //cambiar clock_gettime y nanosleep
{
	struct timeval next_activation;
	struct timeval now, timeout;
	gettimeofday (&next_activation, NULL);
	while (1) {
		struct timeval *period = task_get_period (pthread_self());
		timeval_add (&next_activation, &next_activation, period);
		gettimeofday (&now, NULL);
		timeval_sub (&timeout, &next_activation, &now);
		select (0, NULL, NULL, NULL, &timeout) ;
		fsm_fire (wallet_fsm);
	}
}




int main ()
{
	
	cofm_fsm = fsm_new (cofm);
	wallet_fsm = fsm_new(wallet);

	pthread_t coffe = task_new ("coffe_task", task1_func, 300, 300, 1, 1024);
	pthread_join (coffe, NULL);

	pthread_t wallet = task_new ("wallet_task", task2_func, 400, 400, 2, 1024);
	pthread_join (wallet, NULL);

	init_mutex (&m_scr, 1);

	wiringPiSetup();
	pinMode (GPIO_BUTTON, INPUT);
	pinMode (GPIO_COIN, INPUT);
	wiringPiISR (GPIO_BUTTON, INT_EDGE_FALLING, button_isr);
	wiringPiISR (GPIO_COIN, INT_EDGE_FALLING, coin_isr);
	pinMode (GPIO_CUP, OUTPUT);
	pinMode (GPIO_COFFEE, OUTPUT);
	pinMode (GPIO_MILK, OUTPUT);
	pinMode (GPIO_LED, OUTPUT);
	pinMode (GPIO_CHANGE, OUTPUT);
	digitalWrite (GPIO_LED, HIGH);
	
	while (1) {

		if(scanf("%d %d %d %d", &button,&timer,&coin,&button_change)!=4){
			break;
		}
	}
	
	
	TIME({printf("Tiempo maximo maquina cafe: \n");
	printf("Segundos: %lu \n",cofm_fsm->max_time.tv_sec);
	printf("Nano segundos: %lu \n",cofm_fsm->max_time.tv_nsec);

	printf("Tiempo maximo monedero cafe: \n");
	printf("Segundos: %lu \n",wallet_fsm->max_time.tv_sec);
	printf("Nano segundos: %lu \n",wallet_fsm->max_time.tv_nsec);});


	TIME2({printf("Tiempo maximo can give coffe: \n");
	printf("Segundos: %lu \n",max1.tv_sec);
	printf("Nano segundos: %lu \n",max1.tv_nsec);

	printf("Tiempo maximo coffe given: \n");
	printf("Segundos: %lu \n",max2.tv_sec);
	printf("Nano segundos: %lu \n",max2.tv_nsec);

	printf("Tiempo maximo giving coffe: \n");
	printf("Segundos: %lu \n",max3.tv_sec);
	printf("Nano segundos: %lu \n",max3.tv_nsec);});


	return 0;
}
