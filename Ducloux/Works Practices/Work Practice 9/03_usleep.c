// gcc -o app 03_usleep.c

#include <stdio.h>
#include <unistd.h>

int main() {
    int count=0;

    while(1){
        printf("%d\r\n", count++);
        usleep(1000000);
    }
    return 0;
}

