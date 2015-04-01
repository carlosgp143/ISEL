#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <wiringPi.h>
#include <stdio.h>
#include "fsm.h"
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
	return ret&&can_give_coffe;
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
	return ret&&!giving_coffe;
}


static int money_in (fsm_t* this){
	if(coin){
		coin_in = readCoin();

		money += coin_in; 
		DEBUG(printf("El dinero almacenado es %d\n", money));
		coin=0;
		if(money >= 50){
			can_give_coffe = 1;
			}
		else {
			can_give_coffe = 0;
		}
		return can_give_coffe;

	}
	else{
		return 0;
	}

	
}

static int coffe_finished (fsm_t* this){
	if (coffe_given){
		return 1;
	}
	else{
		return 0;
	}

}


static void cup (fsm_t* this)
{
	DEBUG(printf("Saco la taza\n"));
	giving_coffe = 1;
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
	coffe_given = 1;
}

static void money_enough(fsm_t* this){
	DEBUG(printf("Tengo dinero suficiente\n"));
	can_give_coffe = 1;
}

static void give_change (fsm_t* this){
	digitalWrite(GPIO_CHANGE, HIGH);
	change = money-50;
	DEBUG(printf("Devuelvo el cambio: %d\n", change););
	
	money = 0;
	can_give_coffe= 0;
	coin_in = 0;
	coffe_given = 0;
	giving_coffe = 0;

}

static void give_money (fsm_t* this){
	
	digitalWrite(GPIO_CHANGE, HIGH);
	change = money;
	DEBUG(printf("Devuelvo el dinero: %d\n", change););

	money = 0;
	can_give_coffe= 0;
	coin_in = 0;
	coffe_given = 0;

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

int main ()
{
	//struct timeval clk_period = { 0, 250 * 1000 }; 
	//struct timeval next_activation;
	struct timespec secundary_cycle = {0, 100000000};
	struct timespec rem;
	struct timespec start;
	struct timespec stop;
	struct timespec total;
	struct timespec wait;

	int frame = 0;

	fsm_t* cofm_fsm = fsm_new (cofm);
	fsm_t* wallet_fsm = fsm_new(wallet);

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
	//gettimeofday (&next_activation, NULL);
while (1) {

	if(scanf("%d %d %d %d", &button,&timer,&coin,&button_change)!=4){
		break;
	}
	clock_gettime( CLOCK_MONOTONIC, &start);

	switch (frame){
		case 0: fsm_fire (cofm_fsm);
				fsm_fire(wallet_fsm); 
				break;

		case 1: break;
		case 2: break;

		case 3: fsm_fire (cofm_fsm);
				break;

		case 4: fsm_fire(wallet_fsm);
				break;

		case 5: break;

		case 6: fsm_fire (cofm_fsm);
				break;

		case 7: break;

		case 8: fsm_fire(wallet_fsm);
				break;

		case 9: fsm_fire (cofm_fsm);
				break;
		case 10: break;
		case 11: break;

	}
	clock_gettime( CLOCK_MONOTONIC, &stop);

	frame++;
	if(frame>11) frame = 0;
	
	timespec_sub(&total,&stop,&start);
	timespec_sub(&wait,&secundary_cycle,&total);

	nanosleep(&wait, &rem); //esperamos hasta que se termine el ciclo secundario
	}

	TIME({printf("Tiempo maximo maquina cafe: \n");
	printf("Segundos: %lu \n",cofm_fsm->max_time.tv_sec);
	printf("Nano segundos: %lu \n",cofm_fsm->max_time.tv_nsec);

	printf("Tiempo maximo monedero cafe: \n");
	printf("Segundos: %lu \n",wallet_fsm->max_time.tv_sec);
	printf("Nano segundos: %lu \n",wallet_fsm->max_time.tv_nsec);});

	return 0;
}
