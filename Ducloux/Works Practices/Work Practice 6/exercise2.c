#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Función auxiliar para dormir en milisegundos
void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

// TAREA 1 (100 ms)
void* Task1(void* arg) {
    while (1) {
        printf("[Tarea 1] - 100ms\n");
        sleep_ms(100);
    }
    return NULL;
}

// TAREA 2 (300 ms)
void* Task2(void* arg) {
    while (1) {
        printf("[Tarea 2] - 300ms\n");
        sleep_ms(300);
    }
    return NULL;
}


// TAREA 3 (500 ms)
void* Task3(void* arg) {
    while (1) {
        printf("[Tarea 3] - 500ms\n");
        sleep_ms(500);
    }
    return NULL;
}

int main() {

    printf("Iniciando sistema multihilo...\n");

    pthread_t thread1, thread2, thread3;

    // Crear hilos
    if (pthread_create(&thread1, NULL, Task1, NULL) != 0 ||
        pthread_create(&thread2, NULL, Task2, NULL) != 0 ||
        pthread_create(&thread3, NULL, Task3, NULL) != 0) {
        
        printf("Error al crear los hilos.\n");
        exit(1);
    }

    // Esperar indefinidamente (los hilos nunca terminan)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return 0;
}