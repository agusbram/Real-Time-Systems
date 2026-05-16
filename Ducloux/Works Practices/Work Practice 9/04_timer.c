// gcc -o app 04_timer.c -lrt

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#define TS_NS 0 // nanosec (999999999 max)
#define TS_S 1 // sec

void tickConfig(unsigned int period_sec, unsigned int period_nsec);
void tickHandler(int sig);

int main(int argc, char *argv[])
{
    
    printf("**********\n\r");
    printf("** Tick **\n\r");
    printf("**********\n\r");
	
	
    tickConfig(TS_S, TS_NS);
    
    while(1)
        pause();

    return 0;
}


void tickConfig(unsigned int period_sec, unsigned int period_nsec){
    
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
  

    signal(SIGALRM, tickHandler); 

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
        printf("error timer_create");

    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = period_sec;
    its.it_interval.tv_nsec = period_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
        printf("error timer_settime");
    
    return; 
}


void tickHandler(int sig)
{
    static int i=0;
    printf("muestra: %d\n\r", i);
    i++;
    return;
}

