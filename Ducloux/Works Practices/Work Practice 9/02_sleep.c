// gcc -o app 02_sleep.c

#include <stdio.h>
#include <unistd.h>

int main() {
    int count=0;

    while(1){
        printf("%d\r\n", count++);
        sleep(1);
    }
    return 0;
}

