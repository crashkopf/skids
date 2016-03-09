#include <signal.h>
#include <time.h>

#include "control.h"

timer_t timerid;
struct sigevent sev;
struct itimerspec its;
struct sigaction sa;

static void handler(int sig, siginfo_t *si, void *uc) {
	j5update();
}


int timer_init(void) {
	/* Establish handler for timer signal */
	sa.sa_flags = SA_SIGINFO|SA_RESTART; // Use sa_sigaction handler; retart interrupted I/O
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
		return -1;

	/* Create the timer */
	sev.sigev_notify = SIGEV_SIGNAL; // Notification via signal
	sev.sigev_signo = SIGRTMIN; // Use RealTime signal
	sev.sigev_value.sival_ptr = &timerid; // Pass pointer to timer id to handler
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
		return -1;
	
	return 0;
}

int timer_start(void) {
	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = 0;//100000;
	its.it_interval.tv_sec = 1;
	its.it_interval.tv_nsec = 0;//100000; // 100mS
	
	if (timer_settime(timerid, 0, &its, NULL) == -1)
		return -1;
	
	return 0;
}

int timer_block(void) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN);
	return sigprocmask(SIG_BLOCK, &mask, NULL);
}
	   
int timer_unblock(void) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN);
	return sigprocmask(SIG_UNBLOCK, &mask, NULL);
}