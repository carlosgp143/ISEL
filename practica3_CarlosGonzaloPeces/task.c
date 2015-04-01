#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "task.h"

#define MAXTASKS 20

typedef struct taskdesk_t { //struct que representa una de nuestras tareas
	pthread_t tid; //pthread de xenomai
	const char* name; //nombre de la tarea
	struct timeval period; //periodo de la tarea
	struct timeval deadline; //plazo de la tarea
	int prio; //prioridad de la tarea
} taskdesc_t;

static taskdesc_t desc[MAXTASKS]; //array con las tareas que tenemos creadas

static int ntasks = 0; //numero de tareas creadas

static taskdesc_t* taskdesc_find (pthread_t tid) //busca una de las tareas en el array y nos la devuelve
{
	int i;
	for (i = 0; i < ntasks; ++i)
		if (tid == desc[i].tid)
		return &desc[i];
	return NULL;
}

struct timeval* task_get_period (pthread_t tid) //devuelve el periodo
{
	taskdesc_t* this = taskdesc_find (tid);
	return &this->period;
}

struct timeval* task_get_deadline (pthread_t tid) //devuelve el plazo
{
	taskdesc_t* this = taskdesc_find (tid);
	return &this->deadline;
}

//crea una nueva tarea, asignandola los diferentes parametros e incluyendola en el array de tareas.
pthread_t task_new (const char* name, void *(*f)(void *),int period_ms, int deadline_ms,int prio, int stacksize) 
{
	taskdesc_t* tdesc = &desc[ntasks++];
	pthread_attr_t attr;
	struct sched_param sparam;
	sparam.sched_priority = sched_get_priority_min (SCHED_FIFO) + prio; 
	pthread_attr_init (&attr);
	pthread_attr_setstacksize (&attr, stacksize);
	pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);  //ambito de planificación
	pthread_attr_setschedpolicy (&attr, SCHED_FIFO);   //politica de planificación
	pthread_attr_setschedparam (&attr, &sparam);
	pthread_create (&tdesc->tid, &attr, f, tdesc);
	tdesc->name = name;
	tdesc->prio = prio;
	tdesc->period.tv_sec = period_ms / 1000;
	tdesc->period.tv_usec = (period_ms % 1000) * 1000;
	tdesc->deadline.tv_sec = deadline_ms / 1000;
	tdesc->deadline.tv_usec = (deadline_ms % 1000) * 1000;
	return tdesc->tid;
}

void mutex_init (pthread_mutex_t* m, int prioceiling) //inicia un mutex, para proteger los recursos compartidos
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init (&attr);
	pthread_mutexattr_setprotocol (&attr, PTHREAD_PRIO_PROTECT);  //especifica techo de prioridad o herencia
	pthread_mutex_init (m, &attr);
	pthread_mutex_setprioceiling(m, sched_get_priority_min(SCHED_FIFO) + prioceiling, NULL);
}

//funciones para restar tiempos y todo eso

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
void
timeval_add (struct timeval *res, struct timeval *a, struct timeval *b)
{
res->tv_sec = a->tv_sec + b->tv_sec
+ a->tv_usec / 1000000 + b->tv_usec / 1000000;
res->tv_usec = a->tv_usec % 1000000 + b->tv_usec % 1000000;
}
int
timeval_less (struct timeval *a, struct timeval *b)
{
return (a->tv_sec < b->tv_sec) ||
((a->tv_sec == b->tv_sec) && (a->tv_usec < b->tv_usec));
}
int
timeval_get_ms (struct timeval *a)
{
return a->tv_sec * 1000 + a->tv_usec / 1000;
}
