// Import libraries
#include <pigpio.h> // Contains functions to manipulate GPIO and temporizator
#include <stdio.h> // To print LED state in screen
#include <stdlib.h> // To use general functions
#include <stdint.h> // Define data types of fixed size, this allows to work with data sizes predictable (uint8_t && uint32_t)
#include <signal.h> // To manipulate signals (in this program SIGINT [ctrl + C])

#define LED_GPIO 4 // Define number of pin GPIO where's connected LED
#define PERIOD 500000 // Defines period of ON and OFF of LED ==> 500000 µs = 500 ms
/**
 * @param sig_atomic_t Special type to work with signal handlers
 * volatile indicates to the compiler that the value can change outside of the main flow of the program
 * This is necessary because the variable is modified inside a function that manipulates signals
 */
volatile sig_atomic_t running = 1;

/**
 * Manipulates signal SIGINT (ctrl + C)
 * 1. Prints in console that the program finished
 * 2. Free's up resources of library pigpio
 * 3. Establish the variable running to 0 to finish while loop
 * @param signal to be manipulated by the O.S.
 *  */ 
void handle_signal(int signal);

int main(int argc, char const *argv[]) {
    // Flag to turn LED on or off
    uint8_t led_state = 0;

    // Initializes the library of pigpio
    if(gpioInitialise() < 0) {
        fprintf(stderr, "Failed initializing gpio\n");
        exit(1);
    }
    
    // Store time from the initialization of the library pigpio
    uint32_t previous_time = gpioTick();
    
    // Establish in output the PIN 4 of the Rasperry Pi
    gpioSetMode(LED_GPIO, PI_OUTPUT);

    // Writes initial state of LED in PIN 4 of the Rasperry Pi
    gpioWrite(LED_GPIO, led_state);

    // Handles ctrl + C interruption
    signal(SIGINT, handle_signal);

    // Prints by console actual state of LED before entering while loop
    printf("LED initialized OFF\n");
    
    // This loop finished when handling SIGINT signal
    while(running) {
        // Obtains actual time in microseconds
        uint32_t actual_time = gpioTick();

        // If 500.000 microseconds have passed enters if
        if(actual_time - previous_time >= PERIOD) {
            // Changes state of led, to turn it ON or OFF
            led_state = !led_state;

            // Prints by console actual LED state after changing it
            if(led_state) {
                printf("LED turned ON\n");
            } else {
                printf("LED turned OFF\n");
            } 

            // Applies state to the fisic PIN
            gpioWrite(LED_GPIO, led_state);

            // Updates time waited for the future event (next iteration)
            // This method maintains a constant PERIOD and avoids an temporal error called temporary drift
            previous_time += PERIOD;
        }
    }

    // Free's up resources of library pigpio again, to ensure that resources are released
    gpioTerminate();

    return 0;
}

void handle_signal(int signal) {
    printf("Program terminated\n");
    gpioTerminate();
    running = 0;
}
