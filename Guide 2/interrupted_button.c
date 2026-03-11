// Import libraries
#include <pigpio.h> // Contains functions to manipulate GPIO and temporizator
#include <stdio.h> // To print LED state in screen
#include <stdlib.h> // To use general functions
#include <stdint.h> // Define data types of fixed size, this allows to work with data sizes predictable (uint8_t && uint32_t)
#include <signal.h> // To manipulate signals (in this program SIGINT [ctrl + C])
#include <unistd.h>

#define LED_GPIO 4 // Define number of pin GPIO where's connected LED
#define BUTTON_GPIO 17 // Define number of pin GPIO where's connected button to turn ON or OFF

/**
* @param sig_atomic_t Special type to work with signal handlers
 * volatile indicates to the compiler that the value can change outside of the main flow of the program
 * This is necessary because the variable is modified inside a function that manipulates signals
 */
volatile sig_atomic_t running = 1;

/**
 * Manipulates signal SIGINT (ctrl + C)
 * 1. Prints in console that the program finished
 * 2. Establish the variable running to 0 to finish while loop
 * @param signal to be manipulated by the O.S.
 *  */ 
void handle_signal(int signal);

/**
 * Handles all of the events that occurs when logic level changes in GPIO. This is handled in an asynchronous way.
 * @param gpio number of physical pin of GPI that generated the event handled
 * @param level indicates actual logic level of GPIO in the moment that happens the event handled
 * @param tick time in microseconds that indicates when happened the change of the state of GPIO
 *  */ 
void button_callback(int gpio, int level, uint32_t tick);

int main(int argc, char const *argv[]) {
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

    // Handles ctrl + C interruption
    signal(SIGINT, handle_signal);

    // Prints by console actual state of LED before entering while loop
    printf("LED initialized OFF\n");

    /* Permite registrar una función callback que se ejecuta automáticamente cuando se detecta un cambio de nivel lógico en un GPIO. 
    Este mecanismo evita el uso de polling continuo y permite manejar eventos de hardware de forma reactiva. */
    gpioSetAlertFunc(BUTTON_GPIO, button_callback);
    
    // This loop finished when handling SIGINT signal
    while(running) {
        // Program sleeps 100 miliseconds and then iterates again
        // This avoids busy waiting and mantains the program alive while pigpio handles callbacks
        // Now the program doesn't consumes CPU unnecessarily
        // 100.000 microsecond = 100 miliseconds
        // usleep = sleep but usleep is for microseconds and sleep is for seconds
        usleep(100000);
    }

    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();

    return 0;
}

void button_callback(int gpio, int level, uint32_t tick) {
    // This is not necessary because only exists one button in this program. 
    // This allows the function to be reusable
    if(gpio == BUTTON_GPIO) {
        // If LED is ON
        if(level == 1) {
            // Turns OFF LED
            gpioWrite(LED_GPIO, 0);
            printf("Botón liberado\n");
            // If LED is OFF
        } else if(level == 0) {
            // Turns ON LED
            gpioWrite(LED_GPIO, 1);
            printf("Botón presionado\n");
        }
    }
}

void handle_signal(int signal) {
    printf("Program terminated\n");
    running = 0;
}
