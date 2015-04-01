#include <stdio.h>
#include "reactor.h"

static
void
task1_func (struct event_handler_t* this)
{
	static const struct timeval period = { 0, 300000 };
	static int cnt = 0;
	printf ("%d\n", cnt++);
	timeval_add (&this->next_activation, &this->next_activation, &period);
}

static
void
task2_func (struct event_handler_t* this)
{
	static const struct timeval period = { 0, 400000 };
	printf ("hola\n");
	timeval_add (&this->next_activation, &this->next_activation, &period);
}

int
main ()
{
	EventHandler eh1;
	EventHandler eh2;
	reactor_init ();
	event_handler_init (&eh1, 2, task1_func);
	event_handler_init (&eh2, 1, task2_func);
	reactor_add_handler (&eh1);
	reactor_add_handler (&eh2);
	while (1) {
		reactor_handle_events ();
	}
}