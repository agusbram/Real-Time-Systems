#define _POSIX_C_SOURCE 199506L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

/*
 * ===============================
 * CONFIGURACIÓN GENERAL
 * ===============================
 */

// Cantidad máxima de eventos en la cola
#define MAX_EVENTS 10

/*
 * ===============================
 * ESTRUCTURA DE EVENTO
 * ===============================
 * Cada evento representa una tarea a ejecutar.
 * Incluye:
 * - tarea_id: identifica qué tarea ejecutar
 * - prioridad: define el orden (mayor valor = mayor prioridad)
 */
typedef struct {
    int tarea_id;
    int prioridad;
} evento_t;

/*
 * ===============================
 * COLA DE EVENTOS PRIORIZADA
 * ===============================
 * Se implementa como un arreglo ordenado.
 * Siempre se mantiene ordenado por prioridad (descendente).
 * Las prioridades NO cambian la frecuencia de los LEDs
 * Solo cambian el ORDEN de ejecución cuando hay conflicto
 * El uso de una cola de eventos priorizada no modifica la periodicidad de las tareas, pero 
 * garantiza que, en caso de concurrencia de eventos, las tareas de mayor prioridad
 * (como la asociada al LED1) sean ejecutadas antes, reduciendo su latencia 
 * y mejorando la respuesta del sistema.”
 */

evento_t cola[MAX_EVENTS];
int cola_size = 0;

/*
 * Inserta un evento en la cola respetando prioridad.
 * Se hace un corrimiento de elementos para mantener orden.
 */
void push_evento(int tarea_id, int prioridad) {

    // Evitar overflow de la cola
    if (cola_size >= MAX_EVENTS) {
        return;
    }

    int i = cola_size - 1;

    // Desplazar elementos de menor prioridad
    while (i >= 0 && cola[i].prioridad < prioridad) {
        cola[i + 1] = cola[i];
        i--;
    }

    // Insertar nuevo evento
    cola[i + 1].tarea_id = tarea_id;
    cola[i + 1].prioridad = prioridad;

    cola_size++;
}

/*
 * Extrae el evento de mayor prioridad (posición 0)
 */
evento_t pop_evento() {

    evento_t e = cola[0];

    // Corrimiento hacia la izquierda
    for (int i = 1; i < cola_size; i++) {
        cola[i - 1] = cola[i];
    }

    cola_size--;

    return e;
}

/*
 * ===============================
 * MANEJO DE TIEMPO
 * ===============================
 */

struct timespec start;

/*
 * Devuelve el tiempo transcurrido desde el inicio (en ms)
 */
long get_time_ms() {

    // Valor del tiempo en nanosegundos
    struct timespec now;

    // Se obtiene el tiempo actual con monotonic
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Se realiza la conversión a milisegundos
    return (now.tv_sec - start.tv_sec) * 1000 +
           (now.tv_nsec - start.tv_nsec) / 1000000;
}

/*
 * ===============================
 * DEFINICIÓN DE TAREAS
 * ===============================
 * Cada tarea representa una acción periódica.
 */

void tarea1() {
    printf("Tarea 1 (100 ms) [ALTA]  - t=%ld ms\n", get_time_ms());
}

void tarea2() {
    printf("Tarea 2 (300 ms) [MEDIA] - t=%ld ms\n", get_time_ms());
}

void tarea3() {
    printf("Tarea 3 (500 ms) [BAJA]  - t=%ld ms\n", get_time_ms());
}

/*
 * ===============================
 * CREACIÓN DE TIMERS POSIX
 * ===============================
 * Cada timer genera una señal periódica.
 */

timer_t crear_timer(int signo, int periodo_ms) {

    // Transporta los valores de aplicación definidos a señales
    // Convierte resultados lógicos a señales
    // Define que pasa cuando vence el timer ==> Envía una señal
    struct sigevent sev;

    // Estructura utilizada para valores de timer iniciales e intervalos
    // Define cuándo y cada cuánto se ejecuta el timer
    struct itimerspec its;

    // Identificador del timer
    timer_t timerid;

    // Configurar evento del timer → señal
    sev.sigev_notify = SIGEV_SIGNAL; // Cuando el timer vence ==> Envía una señal
    sev.sigev_signo = signo; // Define que señal
    sev.sigev_value.sival_ptr = &timerid; // Información adicional asociada a la señal

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("Error en timer_create");
        exit(EXIT_FAILURE);
    }

    // Tiempo inicial
    // Convierte milisegundos ==> Segundos y Nanosegundos
    its.it_value.tv_sec = periodo_ms / 1000; 
    its.it_value.tv_nsec = (periodo_ms % 1000) * 1000000;

    // Repetición periódica
    // it_value ==> primer disparo
    // it_interval ==> repetición
    // Ambos parámetros son iguales por lo tanto el timer es periódico
    its.it_interval = its.it_value;

    // Arranca el timer
    // Usa la configuración utilizada en its
    // Timer ahora genera señales automáticamente
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("Error en timer_settime");
        exit(EXIT_FAILURE);
    }

    return timerid;
}

int main() {
    // Guardar tiempo inicial
    // Con CLOCK_MONOTONIC el reloj no se ve afectado por cambios del sistema
    clock_gettime(CLOCK_MONOTONIC, &start);

    /*
     * CONFIGURACIÓN DE SEÑALES
     * Se usan señales en tiempo real:
     * SIGRTMIN, SIGRTMIN+1, SIGRTMIN+2
     */
    sigset_t set; // Crea un conjunto de señales
    sigemptyset(&set); // Inicializa el conjunto

    // Agrega las señales que van a usar los timers
    sigaddset(&set, SIGRTMIN);     // Tarea 1
    sigaddset(&set, SIGRTMIN + 1); // Tarea 2
    sigaddset(&set, SIGRTMIN + 2); // Tarea 3

    // Bloquear señales para usarlas con sigwait()
    // Evita que las señales se manejen automáticamente
    // Sino utilizamos esto ==> las señales interrumpirían el flujo y no se podrían controlar bien
    sigprocmask(SIG_BLOCK, &set, NULL);

    /*
     * CREACIÓN DE TIMERS PERIÓDICOS
     * Ahora el kernel genera señales automáticamente sin intervención del CPU
     */
    crear_timer(SIGRTMIN,     100); // 100 ms
    crear_timer(SIGRTMIN + 1, 300); // 300 ms
    crear_timer(SIGRTMIN + 2, 500); // 500 ms

    printf("Sistema iniciado (cola de eventos priorizada)...\n");

    /*
     * ===============================
     * EJECUTIVO CÍCLICO
     * ===============================
     * - Loop infinito
     * - Sin espera activa (bloqueante)
     * - Manejo de eventos
     */
    while (1) {

        int sig;

        // Espera BLOQUEANTE → no consume CPU
        // El proceso se duerme y no bloquea CPU, se despierta solo cuando llega una señal
        // Cuando un timer vence:
        // 1. El kernel manda una señal
        // 2. sigwait se desbloquea
        // 3. Guarda la señal en sig
        sigwait(&set, &sig);

        /*
         * GENERACIÓN DE EVENTOS
         * Cada señal activa una tarea
         * Se insertan en la cola con prioridad
         * Convierte la señal en un evento lógico (LED's en este caso)
         * Esto permite que el orden de los eventos siempre sea: [T1, T2, T3] no importa el orden de llegada
         * SIGRTMIN: señal de tiempo real en sistemas POSIX GNU/Linux
         */
        if (sig == SIGRTMIN) {
            push_evento(1, 3); // Alta prioridad
        }

        if (sig == SIGRTMIN + 1) {
            push_evento(2, 2); // Media prioridad
        }

        if (sig == SIGRTMIN + 2) {
            push_evento(3, 1); // Baja prioridad
        }

        /*
         * DISPATCHER ==> Es el componente que decide qué tarea ejecutar y cuándo
         * Procesa todos los eventos pendientes
         * respetando el orden de prioridad
         */
        while (cola_size > 0) {
            // Saca el evento mas prioritario
            evento_t e = pop_evento();

            // Ejecuta la tarea correspondiente (Prende LED)
            switch (e.tarea_id) {
                case 1:
                    tarea1();
                    break;
                case 2:
                    tarea2();
                    break;
                case 3:
                    tarea3();
                    break;
            }
        }

        // Flujo Completo:
        // Timer → Señal → sigwait → Evento → Cola → Dispatcher → Tarea
    }

    return 0;
}