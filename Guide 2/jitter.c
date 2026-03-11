// Import libraries
#include <pigpio.h> // Contains functions to manipulate GPIO and temporizator
#include <stdio.h> // To print LED state in screen
#include <stdlib.h> // To use general functions
#include <stdint.h> // Define data types of fixed size, this allows to work with data sizes predictable (uint8_t && uint32_t)
#include <signal.h> // To manipulate signals (in this program SIGINT [ctrl + C])
#include <unistd.h> // To use usleep function
#include <inttypes.h> // To print uint32_t type in console

#define LED_GPIO 4 // Define number of pin GPIO where's connected LED
#define BUTTON_GPIO 17 // Define number of pin GPIO where's connected button to turn ON or OFF
#define N 20 // Quantity of measurements (20 pulsations)

// Measurements of latencias (20 pulsations)
uint32_t latencies[N];

// To store minimum and maximun latencies
uint32_t min, max;

// Count to verify finish of measurements
int count = 0;

/**
* @param sig_atomic_t Special type to work with signal handlers
 * volatile indicates to the compiler that the value can change outside of the main flow of the program
 * This is necessary because the variable is modified inside a function that manipulates signals
 */
volatile sig_atomic_t running = 1;

/**
 * Handles all of the events that occurs when logic level changes in GPIO. This is handled in an asynchronous way.
 * @param gpio number of physical pin of GPI that generated the event handled
 * @param level indicates actual logic level of GPIO in the moment that happens the event handled
 * @param tick time in microseconds that indicates when happened the change of the state of GPIO
 *  */ 
void button_callback(int gpio, int level, uint32_t tick);

/**
 * Calculates minimum and maximum latencies using a comparison 
 * between first element of array and the rest of elements
 * @param latencies latencies to use to calculate minimum, maximum latency and average latency
 * @param N quantity of measurements
 * @param min minimum latency
 * @param max maximum latency
 */
void calculate_min_max(uint32_t latencies[], int N, uint32_t *min, uint32_t *max);

/**
 * Manipulates signal SIGINT (ctrl + C)
 * 1. Prints in console that the program finished
 * 2. Establish the variable running to 0 to finish while loop
 * @param signal to be manipulated by the O.S.
 *  */ 
void handle_signal(int signal);

int main(int argc, char const *argv[]) {
    uint32_t latencies_total = 0;

    // Initializes the library of pigpio
    if(gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }
    
    // Establish in output the PIN 4 of the Rasperry Pi
    gpioSetMode(LED_GPIO, PI_OUTPUT);

    // Establish in input the PIN 17 of the Rasperry Pi
    gpioSetMode(BUTTON_GPIO, PI_INPUT);

    // PIN receives electric sound of the ambience
    // It's necessary to reduce that sound to keep away from float state, this means that GPIO can read 0 1 0 1 1 0 0 1 even if you haven't pressed the button
    // @param PI_PUD_UP (Pull-up) connects PIN to VCC (3.3V) through a resistance.
    // Pull up means:
    //  When button is not pressed ==> pin mantains in 1 logic
    //  When button is pressed ==> pin is connected to GROUND and passes to 0 logic.
    gpioSetPullUpDown(BUTTON_GPIO, PI_PUD_UP);

    // This function allows to pigpio to wait until the change state is stable:
    // time(ms)    GPIO
    // 0             1
    // 1             0
    // 2             1
    // 3             0
    // 4             1
    // 5             0
    // 15            0
    // 10000 microseconds = 10 miliseconds
    // Normal bouncing typical mechanics of buttons is between 1 - 5 miliseconds. In the worst case is aproximately 10 miliseconds.
    gpioSetGlitchFilter(BUTTON_GPIO, 10000);

    // Writes initial state of LED in PIN 4 of the Rasperry Pi
    gpioWrite(LED_GPIO, 0);

    // Prints by console actual state of LED before entering while loop
    printf("LED initialized OFF\n");

    /* Permite registrar una función callback que se ejecuta automáticamente cuando se detecta un cambio de nivel lógico en un GPIO. 
    Este mecanismo evita el uso de polling continuo y permite manejar eventos de hardware de forma reactiva. */
    gpioSetAlertFunc(BUTTON_GPIO, button_callback);

    // This loop finished when handling SIGINT signal
    while(count < N && running) {
        // Program sleeps 100 miliseconds and then iterates again
        // This avoids busy waiting and mantains the program alive while pigpio handles callbacks
        // Now the program doesn't consumes CPU unnecessarily
        // 100.000 microsecond = 100 miliseconds
        // usleep = sleep but usleep is for microseconds and sleep is for seconds
        usleep(100000);

    }
    
    // This finishes reading interruptions/events of button
    gpioSetAlertFunc(BUTTON_GPIO, NULL);

    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();

    // Print it in case program finished with signal SIGINT
    printf("Quantity of pulsations = %d\n", count);

    // Print first latency
    printf("Initial Latency [first_time_led - first_time_btn] = " PRIu32 "\n", (latencies[0]));

    // Calculate total of latency
    for(int t = 0; t < count; t++) {
        latencies_total = latencies_total + latencies[t];
    }
    
    // Avoid using 0 measurements
    if(count > 0) {
        // Calculates minimum and maximum latencies
        calculate_min_max(latencies, count, &min, &max);

        // Print minimum and maximum latencies
        printf("Maximum latency = " PRIu32 "\nMinimum latency = " PRIu32 "\n", max, min);
    
        // Print average latency
        printf("Average latency = " PRIu32 "\n", (latencies_total / count));

        // Calculate and print variability (Jitter)
        printf("Jitter [Maximum latency - minimum latency] = " PRIu32 "\n", (max - min));

        /* 
        Only for debugging:
        printf("All latencies:\n");
        for(int i = 0; i < count; i++) {
            printf("[%d] = %" PRIu32 " us\n", i+1, latencies[i]);
        } */
    } else {
        printf("No latencies and measurements recorded.\n");
    }

    // To see the difference if program terminated with CTRL + C or because the 20 measurements were terminated
    if(!running) {
        printf("Program terminated by SIGINT\n");
    } else {
        printf("Program finished normally\n");
    }

    return 0;
}

void calculate_min_max(uint32_t latencies[], int N, uint32_t *min, uint32_t *max) {
    *min = latencies[0];
    *max = latencies[0];

    for(int i = 1; i < N; i++) {
        // Calculate minimum
        if(latencies[i] < *min) {
            *min = latencies[i];
        }

        // Calculate maximum
        if(latencies[i] > *max) {
            *max = latencies[i];
        }
    }
}

void button_callback(int gpio, int level, uint32_t tick) {
    // This is not necessary because only exists one button in this program. 
    // This allows the function to be reusable
    if(gpio == BUTTON_GPIO) {
        // Measures time that happens button's interruption in microseconds
        uint32_t time_btn_event = tick;

        // If LED is ON
        if(level == 1) {
            // Turns OFF LED
            gpioWrite(LED_GPIO, 0);

            // Dont print in console because delays real time measurements and events
            // printf("Botón liberado\n");

            // If LED is OFF
        } else if(level == 0) {
            // Measures time that happens event when led is turned ON in microseconds
            uint32_t time_led_on = gpioTick();

            // Turns ON LED
            gpioWrite(LED_GPIO, 1);

            // Dont print in console because delays real time measurements and events
            // printf("Botón presionado\n");

            // Maximum 20 measurements
            if(count < N) {
                // Formula of latency
                latencies[count] = time_led_on - time_btn_event;

                // To keep storing latencies correctly
                count = count + 1;
            }
        }
    }
}

void handle_signal(int signal) {
    printf("Program terminated\n");
    running = 0;
}
