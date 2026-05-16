// gcc -o app 01_alarm.c

#include <signal.h>
#include <stdio.h>
#include <unistd.h>


void handle_alarm(int sig) {
    static int count=0;
    printf("%d\r\n", count++);
}

int main() {

    signal(SIGALRM, handle_alarm);
    
    alarm(1);
    
    while(1){
        pause();
        alarm(1);
    }
    
    return 0;
}

