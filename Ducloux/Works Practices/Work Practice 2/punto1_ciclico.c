#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

// Definimos una variable global que el Timer incrementará.
// Será nuestro "reloj" del sistema. (1 Tic = 1 milisegundo)
volatile unsigned long Tics = 0;

// Manejador de la interrupción del Timer (ISR)
void timer_handler(int signum) {
    Tics++;
}

// Configuración del timer del Sistema Operativo para que interrumpa cada 1 ms
void setup_timer() {
    struct sigaction sa;
    struct itimerval timer;

    sa.sa_handler = &timer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000; // 1 ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000; // 1 ms

    setitimer(ITIMER_REAL, &timer, NULL);
}

// Tarea 1: Período de 100 ms
void Task1(void) {
    static int state = 0;
    static unsigned long timer;
    
    switch (state) {
        case 0: // Inicialización
            timer = Tics + 100; 
            state = 1;
            break;
        case 1: // Ejecución periódica
            if (Tics >= timer) {
                printf("[Tarea 1] - 100ms - Tiempo transcurrido: %lu ms\n", Tics);
                timer = Tics + 100; // Recargamos para la próxima ejecución
            }
            break;
    }
}

// Tarea 2: Período de 300 ms
void Task2(void) {
    static int state = 0;
    static unsigned long timer;
    
    switch (state) {
        case 0:
            timer = Tics + 300;
            state = 1;
            break;
        case 1:
            if (Tics >= timer) {
                printf("[Tarea 2] - 300ms - Tiempo transcurrido: %lu ms\n", Tics);
                timer = Tics + 300;
            }
            break;
    }
}

// Tarea 3: Período de 500 ms
void Task3(void) {
    static int state = 0;
    static unsigned long timer;
    
    switch (state) {
        case 0:
            timer = Tics + 500;
            state = 1;
            break;
        case 1:
            if (Tics >= timer) {
                printf("[Tarea 3] - 500ms - Tiempo transcurrido: %lu ms\n", Tics);
                timer = Tics + 500;
            }
            break;
    }
}

int main() {
    printf("Iniciando Ejecutivo Ciclico...\n");
    setup_timer(); // Arrancamos el timer en hardware/OS

    // Súper-bucle infinito del ejecutivo cíclico
    while (1) {
        Task1();
        Task2();
        Task3();
        
        // Podemos poner una pequeña pausa (ej. 100 microsegundos) 
        // para no saturar el 100% de la CPU en polling.
        usleep(100); 
    }

    return 0;
}
