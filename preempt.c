#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

#include "preempt.h"
#include "uthread.h"

//We will be implementing preemption using five functions.
/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100
#define TEN_THOUSAND 10000

//creating an instance of sigaction.
struct sigaction action;

//creating an instance of settimer.

struct itimerval old, new;

//disables preemption by adding a block to our mask set.
void preempt_disable(void)
{
	sigprocmask(SIG_BLOCK, &action.sa_mask, NULL);
}

//enables preemption by removing a block from our mask set.
void preempt_enable(void)
{
	sigprocmask(SIG_UNBLOCK, &action.sa_mask, NULL);
}

/*my_alarm() will create a timer that fires alarms 100 times a second
by creating intervals of time from 0 to 10,000 microseconds.*/
unsigned int my_alarm(void)
{
	struct itimerval old, new;
	new.it_interval.tv_sec = 0;
	new.it_interval.tv_usec = TEN_THOUSAND;
	new.it_value.tv_sec = 0;
	new.it_value.tv_usec = TEN_THOUSAND;

	if(setitimer (ITIMER_VIRTUAL, &new, &old) < 0)
	{
		printf("settimer error");
		return 0;
	}
	else
	{
		return old.it_value.tv_sec;
	}
}

//timer_handler will force yield upon the running thread
void timer_handler(int signum)
{
	uthread_yield();
}

/*preempt_start will start our preemption by adding a handler @timer_handler
and virtual alarm @SIGVTALRM to our instance of sigaction and finally it
calls our timer which will fire an alarm 100 times a second*/
void preempt_start(void)
{
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGVTALRM);
	sigprocmask(SIG_UNBLOCK, &action.sa_mask, NULL);
	action.sa_handler = (__sighandler_t)timer_handler;
	sigaction(SIGVTALRM, &action, 0);
	my_alarm();
}
